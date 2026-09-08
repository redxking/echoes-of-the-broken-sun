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
        # Longitudinal ridge and broad shoulders, then a swept needle point.
        # Plate edges have unequal fracture breaks; detail follows the plate's flow.
        stations=[(-.50,.11,.08),(-.35,.36,.62),(-.06,.49,1.),(.23,.32,.60),(.54,.025,.025)]
        transverse=[-1,-.48,0,.51,1]
        if not fine:
            stations=[stations[0],stations[2],stations[-1]];transverse=[-1,0,1]
        grid=[]
        for j,(x,w,h) in enumerate(stations):
            row=[]
            for k,t in enumerate(transverse):
                shift=rng.uniform(-.035,.035) if abs(t)==1 else 0
                z=rise*h*(1-abs(t)**1.1)*(.96+rng.uniform(-.04,.04))
                row.append(add(center,add(mul(u,(x+shift)*length),add(mul(v,t*w*width),mul(n,z)))))
            grid.append(row)
        faces=[]
        for j in range(len(grid)-1):
            for k in range(len(transverse)-1):
                a,b,c,d=grid[j][k],grid[j+1][k],grid[j+1][k+1],grid[j][k+1]
                faces.extend([[a,b,c],[a,c,d]])
        boundary=grid[0]+[row[-1] for row in grid[1:]]+list(reversed(grid[-1][:-1]))+[row[0] for row in reversed(grid[1:-1])]
        bottom=add(center,mul(n,-max(1.,rise*.12)))
        lower=[add(p,mul(n,-max(.7,rise*.07))) for p in boundary]
        for i,p in enumerate(boundary):
            j=(i+1)%len(boundary)
            faces.extend([[p,lower[i],lower[j],boundary[j]],[lower[j],lower[i],bottom]])
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

    # Five primary shields establish a large-to-small hierarchy. Secondary leaves
    # wrap the mantle, rather than multiplying equal shingles across the roof.
    profiles=[(-102,153,84,44),(-65,167,98,68),(-30,180,116,88),(5,184,132,86),(53,148,102,52)]
    for i,(x,z,length,width) in enumerate(profiles):
        broad=width*(1.0 if not heavy else 1.16)
        comp=f'shell_{i+1:02d}'
        plate((x,0,z),length,broad,16,comp,u=(1,0,-.035))
        for side in (-1,1):
            plate((x+4,side*broad*.28,z-8),length*.90,broad*.63,11,comp,
                  u=(1,side*.19,-.30),v=(0,1,-side*.88))
            if fine and i in (1,3):
                plate((x-20,side*broad*.18,z+5),length*.43,broad*.32,4,comp,
                      u=(1,side*.12,-.06))
            if fine:
                plate((x+13,side*broad*.26,z-3),length*.43,.85,.3,
                      f"seam_{i+1:02d}_{'l' if side<0 else 'r'}",u=(1,side*.12,-.2),slot=ember)
        if heavy:
            plate((x-8,0,z+23),length*.90,broad*.88,15,f'molt_plate_{i+1:02d}')

    # The keel is tapered and segmented, exposed only between the large shell shields.
    segment((-119,0,145),(76,0,136),[(0,16),(.30,35),(.72,30),(1,13)],'underbody')
    for side in (-1,1):
        for j in range(3 if fine else 2):
            plate((-95+j*54,side*30,143),55,33,8,'underbody',u=(.95,side*.1,-.25),v=(0,1,side*.8))

    # The lower prow sweeps under the caster. It must not obscure the muzzle.
    plate((86,0,143),98,58,13,'prow',u=(1,0,-.30))
    for side in (-1,1):
        plate((88,side*21,148),95,33,9,'prow',u=(1,side*.09,-.38),v=(0,1,-side*.3))

    # Shoulder-embedded paired mineral rails leave a deliberate amber firing channel.
    # Muzzle remains at the contracted socket x=94, z=176. The prow stays beneath it.
    striker=state=='striker_molt'
    for side in (-1,1):
        plate((48,side*15,177),94,23,10,'caster_housing',u=(1,side*.025,-.01))
        plate((49,side*17,169),90,22,8,'caster_housing',u=(1,0,.04),v=(0,1,-side*.3))
        if fine:
            plate((67,side*7,176),52,1.7,.4,'caster_slot',u=(1,0,0),slot=ember)
    # A fractured aperture insert, not an engineered square muzzle block.
    aperture=[(-5.5,-1.2),(-3.4,-2.1),(4.2,-1.5),(5.8,.8),(2.2,1.7),(-4.4,1.3)]
    front=[(94,y,176+z) for y,z in aperture];back=[(92,y,176+z) for y,z in aperture]
    faces=[front,list(reversed(back))]
    for i in range(6):
        j=(i+1)%6;faces.append([front[i],back[i],back[j],front[j]])
    m.add_convex_solid(faces,ember,'caster_slot')
    if striker:
        for side,tag in ((-1,'l'),(1,'r')):
            plate((49,side*29,180),85,25,11,f'molt_striker_vane_{tag}',u=(1,side*.18,.03))

    for tag,ax,ay,fx,fy in cfg.LEGS:
        side=1 if ay>0 else -1
        joints={n:h for n,parent,h,why in cfg.BONES}
        hip=joints[tag+'_upper'];knee=joints[tag+'_lower'];ankle=joints[tag+'_foot']
        kx,ky,kz=knee
        # Encasing shoulders and knees mask mechanical hinge-like joints.
        plate((ax,ay,148),65,44,18,f'{tag}_hip',u=(.55,side*.83,-.1),v=(-side*.83,.55,0))
        segment(hip,knee,[(0,14),(.28,25),(.72,15),(1,10)],f'{tag}_upper')
        plate((kx+5,ky,kz+4),35,27,9,f'{tag}_knee')
        segment(knee,ankle,[(0,12),(.22,16),(.66,10),(1,5)],f'{tag}_lower')
        for part,p0,p1,num,ww in [('upper',hip,knee,1,48),('lower',knee,ankle,2 if fine else 1,24)]:
            axis=norm(sub(p1,p0));v=norm(cross((0,1,0),axis))
            if cross(axis,v)[1]*side<0:v=mul(v,-1)
            for j in range(num):
                t=.26+.36*j/max(1,num-1)
                c=add(add(p0,mul(sub(p1,p0),t)),(0,side*(19 if part=='upper' else 11),0))
                plate(c,math.sqrt(sum(q*q for q in sub(p1,p0)))*(.66 if part=='lower' else 1.10),ww*(1-.45*t),6,
                      f'{tag}_{part}',u=axis,v=v)
        # Separate toes grow from a common foot root; their lowest point stays above ground.
        for toe in (-1,0,1):
            p=(fx+8,fy+toe*5,3.1)
            plate(p,33,8,4,f'{tag}_foot',u=(1,toe*.16,-.12))
        segment(ankle,(fx+9,fy,4),[(0,6),(.6,7),(1,4)],f'{tag}_foot')
        plate((fx+5,fy,18),32,15,4,f'{tag}_foot',u=(.35,0,-1),v=(0,1,0))

    m.collision.append(kit.CollisionBox('body',(-16,0,cfg.H-46),(cfg.BODY_LEN*.72,cfg.BODY_W+16,78)))
    return m
