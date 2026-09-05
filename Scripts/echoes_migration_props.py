"""Original M02 migration-lane stonework. Author: Angelis Pseftis.

Cosmetic observation sills, maintained rooting margins and foot-polished stone.
No symbols, powered apparatus, actors, collision or implied ecological signals.
"""
from __future__ import annotations

from echoes_evacuation_props import solid, angled_beam

REVISION = "m02-migration-props-v2"
KINDS = ("ObservationSill", "RootingShoulder", "PassagePaving")


def stone(mesh, outline, z, height, material, inset=.90, shift=(0., 0.), crest=None):
    """A convex, asymmetric slab with a supported inset crown."""
    n = len(outline)
    lower = [(x, y, z) for x, y in outline]
    upper = [(x*inset+shift[0], y*inset+shift[1], z+height*(crest[i] if crest else 1.))
             for i,(x,y) in enumerate(outline)]
    center = tuple(sum(p[axis] for p in upper)/n for axis in range(3))
    faces = [tuple(reversed(range(n)))]
    faces += [(n+i,n+(i+1)%n,2*n) for i in range(n)]
    for i in range(n):
        j=(i+1)%n
        faces.extend(((i,j,n+j),(i,n+j,n+i)))
    solid(mesh, lower+upper+[center], faces, material)


def profile(width, depth, x=0., y=0.):
    return [(x+px*width, y+py*depth) for px, py in
            ((-.50,-.26),(-.31,-.50),(.28,-.47),(.50,-.17),
             (.44,.32),(.18,.50),(-.30,.43),(-.48,.16))]


def observation_sill(mesh, high):
    # A six-metre geological ledge occupies three blocked cells. Irregular crest
    # heights and embedded slabs replace a repeated column/marker silhouette.
    stone(mesh, profile(598,188), 0, 19, 0, .98)
    stone(mesh, profile(562,166,-5,0), 19, 81, 0, .91,
          crest=(.52,.64,.93,1.,.82,.72,.89,.64))
    stone(mesh, profile(432,79,-28,40), 52, 69, 1, .87,
          crest=(.63,.72,.96,.91,.67,.77,.85,.70))
    stone(mesh, profile(220,46,-119,-35), 20, 74, 2, .96)
    stone(mesh, profile(198,45,142,-31), 20, 82, 3, .96)
    if high:
        for x,y,z,length in ((-146,-34,94.04,45),(-78,-35,94.04,37),(135,-31,102.04,52)):
            angled_beam(mesh,length,2.5,z,.12,1,x,y,.14)


def rooting_shoulder(mesh, high):
    # Broad occupied footing and maintained mineral seams suggest where repeated
    # rooting was supported. This is surrounding stone, never a usable Waystone.
    stone(mesh, profile(188,174),0,27,0,.96)
    stone(mesh, profile(150,124),27,37,1,.94)
    stone(mesh, profile(132,92),64,9,2,.96)
    for x,y,height,width in ((-57,7,44,32),(52,21,35,38),(-17,49,39,44)):
        stone(mesh,profile(width,54,x,y),73,height,0,.90)
        stone(mesh,profile(width*.74,36,x*.9,y*.9),73+height,2.5,3,.94)
    if high:
        for x in (-20,12):
            angled_beam(mesh,47,3,73.04,.10,1,x,-13,.12)


def passage_paving(mesh, high):
    # Irregular slabs embedded flush in the walking plane, with no manufactured
    # tile pattern or directional/interactive marking.
    stone(mesh,profile(196,194),0,1.4,0,.99)
    stone(mesh,profile(176,69,1,-51),1.4,1.3,2,.99)
    stone(mesh,profile(184,68,-3,16),1.4,1.15,3,.99)
    stone(mesh,profile(141,38,8,67),1.4,.9,1,.99)
    if high:
        for x,y,z in ((-55,-50,2.7),(25,17,2.55),(9,67,2.3)):
            angled_beam(mesh,26,1.3,z+.04,.08,1,x,y,.13)


def build(mesh, high, kind):
    {"ObservationSill":observation_sill,
     "RootingShoulder":rooting_shoulder,
     "PassagePaving":passage_paving}[kind](mesh,high)
