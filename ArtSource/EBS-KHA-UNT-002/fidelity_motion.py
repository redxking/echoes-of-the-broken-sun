"""Foot-targeted pilot motion; source evidence only. Author: Angelis Pseftis."""
import math

def limb_angles(base,tag,ankle_x,ankle_z,body_z):
    bones={n:h for n,p,h,why in base.BONES}
    h=bones[tag+'_upper'];k=bones[tag+'_lower'];a=bones[tag+'_foot']
    l1=math.hypot(k[0]-h[0],k[2]-h[2]);l2=math.hypot(a[0]-k[0],a[2]-k[2])
    dx=ankle_x-h[0];dz=ankle_z-body_z-h[2]
    distance=math.hypot(dx,dz)
    if not abs(l1-l2)+1e-5 < distance < l1+l2-1e-5:
        raise ValueError(f'{tag}: unreachable ankle target {distance:.3f}, reach {abs(l1-l2):.3f}..{l1+l2:.3f}')
    rest1=math.atan2(k[2]-h[2],k[0]-h[0]);rest2=math.atan2(a[2]-k[2],a[0]-k[0])
    rest_delta=math.atan2(math.sin(rest2-rest1),math.cos(rest2-rest1))
    delta=math.copysign(math.acos(max(-1,min(1,(distance*distance-l1*l1-l2*l2)/(2*l1*l2)))),rest_delta)
    theta1=math.atan2(dz,dx)-math.atan2(l2*math.sin(delta),l1+l2*math.cos(delta))
    upper=math.degrees(theta1-rest1)
    lower=math.degrees(delta-rest_delta)
    return upper,lower,-upper-lower

def clips(base):
    result=[]
    # Each key targets the original ankle in world space. No translating feet away
    # from their limb ends to conceal floor penetration. Root never translates.
    for name,duration in [('idle',4.),('move',1.2),('fire_on_the_move',1.2),('molt',4.),('death',2.)]:
        clip=base.skel.AnimationClip(name,duration,loop=name in ('idle','move','fire_on_the_move'),
                                     purpose='Fidelity pilot: grounded foot targets; runtime travel remains external')
        samples=48
        for i in range(samples+1):
            u=i/samples;t=u*duration
            moving=name in ('move','fire_on_the_move')
            body_z=-12. if moving else 0.
            if name=='idle':body_z=-1.5*math.sin(math.pi*u)**2
            if name=='molt':body_z=-36.*math.sin(math.pi*u)**2
            if name=='death':body_z=-50.*(u*u*(3-2*u))
            clip.key('body',t,translation_cm=(0,0,body_z))
            for tag,ax,ay,fx,fy in base.LEGS:
                phase=(u+(0 if tag in ('fl','rr') else .5))%1
                step=math.sin(2*math.pi*phase) if moving else 0
                x=fx+12*step
                z=26.+(9.*max(0.,math.sin(2*math.pi*phase)) if moving else 0)
                up,lo,foot=limb_angles(base,tag,x,z,body_z)
                clip.key(tag+'_upper',t,(up,0,0));clip.key(tag+'_lower',t,(lo,0,0));clip.key(tag+'_foot',t,(foot,0,0))
            if name=='fire_on_the_move':
                recoil=max(0.,1-abs(u-.22)/.12)
                clip.key('caster_pitch',t,(-3*recoil,0,0))
            elif name=='sidestep':
                clip.key('caster_yaw',t,(0,5*math.sin(math.pi*u),0))
        result.append(clip)
    return result
