"""Source-authored mineral forms for the Riftstalker fidelity pilot.

Author and creator: Angelis Pseftis.
Deterministic closed faceted plates, tapered limb cores and joint covers. These
are modeled surfaces, not displacement of the previous blockout. Existing
component identities retain their rig, sockets and adaptation-channel meanings.
"""
import math
import random
import hashlib


def build(kit, cfg, lod, state):
    m = kit.Mesh(cfg.ASSET)
    rock, ember = m.slot(cfg.STRATA), m.slot(cfg.AMBER)
    fine = lod == 0
    heavy = state == 'carapace_molt'

    def add(a, b): return tuple(x+y for x,y in zip(a,b))
    def mul(a, f): return tuple(x*f for x in a)
    def sub(a, b): return tuple(x-y for x,y in zip(a,b))
    def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    def norm(a):
        n=math.sqrt(sum(x*x for x in a));return mul(a,1/n)

    def plate(center, length, width, rise, component, u=(1,0,0), v=(0,1,0), slot=rock):
        """Closed asymmetric lanceolate plate with broad planar facets and a sharp lip."""
        rng=random.Random(hashlib.sha256(repr((component,center,length,width,rise,u,v)).encode()).digest())
        u,v=norm(u),norm(v); n=norm(cross(u,v))
        outline=[(-.50,-.13),(-.39,-.40),(-.12,-.52),(.25,-.32),(.54,-.02),(.24,.32),(-.10,.48),(-.41,.29)]
        if not fine: outline=outline[::2]
        rim=[]
        for x,y in outline:
            jitter=1+rng.uniform(-.07,.07)
            rim.append(add(center,add(mul(u,x*length),mul(v,y*width*jitter))))
        peak=add(center,add(mul(u,-length*.13),mul(n,rise)))
        inner=[add(center,add(mul(sub(p,center),.83),mul(n,rise*.36))) for p in rim]
        bottom=add(center,mul(n,-max(1.,rise*.16)))
        faces=[]
        for i,p in enumerate(rim):
            q=rim[(i+1)%len(rim)]
            j=(i+1)%len(rim)
            faces.extend([[p,q,inner[j],inner[i]],[inner[i],inner[j],peak],[q,p,bottom]])
        m.add_convex_solid(faces,slot,component)

    def segment(p0,p1,radii,component):
        """Tapered angular limb with a bulged muscle/root and narrow mineral tendon."""
        axis=norm(sub(p1,p0)); ref=(0,0,1) if abs(axis[2])<.93 else (0,1,0)
        u=norm(cross(axis,ref));v=norm(cross(axis,u)); count=7 if fine else 5
        rings=[]
        for t,r in radii:
            c=add(p0,mul(sub(p1,p0),t))
            rings.append([add(c,add(mul(u,r*math.cos(2*math.pi*i/count)),mul(v,r*.72*math.sin(2*math.pi*i/count)))) for i in range(count)])
        faces=[list(reversed(rings[0])),rings[-1]]
        for a,b in zip(rings,rings[1:]):
            for i in range(count): faces.append([a[i],a[(i+1)%count],b[(i+1)%count],b[i]])
        # Each segment is convex within individual bands; orient by axial radial direction.
        m.add_convex_solid(faces,rock,component)

    # Five overlapping macro shells remain recognizable beneath smaller scales.
    for i in range(cfg.CARAPACE_SHELLS):
        x,z,length,width,height=cfg.shell_profile(i)
        z=[150.,171.,179.,168.,156.][i]
        width=[61.,82.,94.,82.,62.][i]
        broad=width*(1.12 if not heavy else 1.30)
        comp=f'shell_{i+1:02d}'
        plate((x,0,z+4),length,broad,18,comp)
        rows=2 if fine else 1
        for row in range(rows):
            for side in (-1,1):
                yy=side*broad*(.29+.05*row)
                plate((x-9+row*19+(i%2)*7,yy,z+2-row*13),length*(.73-.12*row),broad*.61,8,comp,
                      u=(1,side*.18,-.26),v=(0,1,-side*.85))
        if fine:
            for side,tag in ((-1,'l'),(1,'r')):
                plate((x+8,side*broad*.30,z+4),length*.46,1.5,.5,f'seam_{i+1:02d}_{tag}',slot=ember)
        if fine:
            for side in (-1,1):
                for j in range(2):
                    plate((x-23+j*26,side*broad*(.13+.06*j),z+17-j*4),length*.50,broad*.37,4.5,comp,
                          u=(1,side*(.12+j*.08),-.08),v=(0,1,side*.15))
        if heavy:
            plate((x-5,0,z+27),length*.82,broad*.76,13,f'molt_plate_{i+1:02d}')

    # The keel is tapered and segmented, exposed only between the large shell shields.
    segment((-119,0,145),(76,0,136),[(0,16),(.30,35),(.72,30),(1,13)],'underbody')
    for side in (-1,1):
        for j in range(5 if fine else 3):
            plate((-105+j*34,side*30,143),55,33,8,'underbody',u=(.95,side*.1,-.25),v=(0,1,side*.8))

    # Prow: one integrated layered mineral blade rather than a pyramidal head.
    plate((95,0,152),94,62,17,'prow',u=(1,0,-.16))
    for side in (-1,1):
        plate((103,side*17,163),83,31,8,'prow',u=(1,-side*.11,-.12))
        if fine: plate((108,side*9,165),28,.7,.3,'prow_seam',u=(1,0,-.12),slot=ember)

    # Caster is an inset shoulder slot between sculpted protective leaves.
    striker=state=='striker_molt'
    for side in (-1,1):
        plate((cfg.CASTER_X+17,side*9,cfg.CASTER_Z),85+(16 if striker else 0),30,6,
              'caster_housing',u=(1,side*.08,-.035))
    m.box((cfg.CASTER_X+43,0,cfg.CASTER_Z),(5,13,4),ember,'caster_slot')
    if striker:
        for side,tag in ((-1,'l'),(1,'r')):
            plate((cfg.CASTER_X+26,side*25,cfg.CASTER_Z+5),65,22,12,f'molt_striker_vane_{tag}',u=(1,side*.25,0))

    for tag,ax,ay,fx,fy in cfg.LEGS:
        side=1 if ay>0 else -1
        ky=side*cfg.KNEE_OUT;kx=(ax+fx)/2
        hip=(ax,ay,cfg.H-56);knee=(kx,ky,cfg.KNEE_Z);ankle=(fx,fy,26.)
        # Encasing shoulders and knees mask mechanical hinge-like joints.
        plate((ax,ay,148),65,44,18,f'{tag}_hip',u=(.55,side*.83,-.1),v=(-side*.83,.55,0))
        segment(hip,knee,[(0,14),(.28,25),(.72,15),(1,10)],f'{tag}_upper')
        plate((kx+5,ky,cfg.KNEE_Z+4),45,32,11,f'{tag}_knee')
        segment(knee,ankle,[(0,12),(.22,16),(.66,10),(1,5)],f'{tag}_lower')
        for part,p0,p1,num,ww in [('upper',hip,knee,2 if fine else 1,43),('lower',knee,ankle,3 if fine else 2,25)]:
            axis=norm(sub(p1,p0));v=norm(cross((0,1,0),axis))
            if cross(axis,v)[1]*side<0:v=mul(v,-1)
            for j in range(num):
                t=.13+.70*j/max(1,num-1)
                c=add(add(p0,mul(sub(p1,p0),t)),(0,side*(19 if part=='upper' else 11),0))
                plate(c,math.sqrt(sum(q*q for q in sub(p1,p0)))*(.58 if part=='lower' else .80),ww*(1-.45*t),6,
                      f'{tag}_{part}',u=axis,v=v)
        # Separate toes grow from a common foot root; their lowest point stays above ground.
        for toe in (-1,0,1):
            p=(fx+8,fy+toe*5,2.23)
            plate(p,33,8,4,f'{tag}_foot',u=(1,toe*.16,-.12))
        segment(ankle,(fx+9,fy,4),[(0,6),(.6,7),(1,4)],f'{tag}_foot')

    m.collision.append(kit.CollisionBox('body',(-16,0,cfg.H-46),(cfg.BODY_LEN*.72,cfg.BODY_W+16,78)))
    return m
