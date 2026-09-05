"""Original Soryn environment geometry. Author: Angelis Pseftis.

Recipes are deterministic and expressed in centimetres. Tall forms fit inside
one blocked tile; walking surfaces remain below 20 cm. No gameplay data lives here.
"""
from __future__ import annotations

import math

REVISION = "soryn-world-kits-v9"
KINDS = ("Basalt", "Shivergrass", "Cavern", "Civic", "Choir", "Solar")


def solid(mesh, vertices, faces, material=0):
    """Faceted solid using Unreal winding, per-triangle normals and planar UVs.

    Fractured cap polygons can be nonplanar. Their individual triangle normals
    must not inherit the first triangle's steep slope across the whole cap.
    """
    import unreal
    points, normals, uvs, triangles = [], [], [], []
    for face in faces:
        polygon = [vertices[index] for index in face]
        face_normal = [0., 0., 0.]
        for a, b in zip(polygon, polygon[1:] + polygon[:1]):
            face_normal[0] += (a[1]-b[1])*(a[2]+b[2])
            face_normal[1] += (a[2]-b[2])*(a[0]+b[0])
            face_normal[2] += (a[0]-b[0])*(a[1]+b[1])
        axis = max(range(3), key=lambda index: abs(face_normal[index]))
        for index in range(1, len(face)-1):
            a, b, c = polygon[0], polygon[index], polygon[index+1]
            u, v = [b[j]-a[j] for j in range(3)], [c[j]-a[j] for j in range(3)]
            normal = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
            length = math.sqrt(sum(n*n for n in normal))
            if length < 1e-8:
                raise ValueError("Degenerate world-kit triangle")
            start = len(points)
            for p in (a, b, c):
                points.append(unreal.Vector(*p))
                normals.append(unreal.Vector(*(n/length for n in normal)))
                uv = (p[1],p[2]) if axis == 0 else (p[0],p[2]) if axis == 1 else (p[0],p[1])
                uvs.append(unreal.Vector2D(uv[0]/200., uv[1]/200.))
            # GeometryCore/VectorUtil.h defines UE normals as edge2.Cross(edge1).
            triangles.append(unreal.IntVector(start, start+2, start+1))
    mesh.append_buffers_to_mesh(unreal.GeometryScriptSimpleMeshBuffers(
        vertices=points, normals=normals, uv0=uvs, triangles=triangles), material_id=material)


def rock(mesh, radius, height, seed, material=0, center=(0.,0.,0.), sides=9):
    """Sheared bedrock: broad fracture planes, undercut courses and a split crown."""
    vertices = []
    levels = (0., .12, .31, .36, .70, 1.)
    widths = (1., 1.015, .88, .94, .79, .59)
    for level, fraction in enumerate(levels):
        for i, (x,y) in enumerate(outline(radius, seed, sides)):
            # Shared slip direction preserves a coherent geological formation.
            dx = radius*.12*math.sin(seed) * fraction
            dy = radius*.10*math.cos(seed*.71) * fraction
            fracture = 0 if level == 0 else height*(.055*math.sin(i*2.3+seed) + .025*math.cos(i*.9+level))
            vertices.append((center[0]+x*widths[level]+dx,
                             center[1]+y*widths[level]+dy,
                             center[2]+fraction*height+fracture))
    solid(mesh,vertices,[tuple(reversed(range(sides))), tuple(range(5*sides,6*sides))],material)
    for level in range(len(levels)-1):
        faces = []
        for i in range(sides):
            j=(i+1)%sides
            faces.append((level*sides+i,level*sides+j,(level+1)*sides+j,(level+1)*sides+i))
        # A recessed dark seam separates the two major strata; no emissive trim.
        solid(mesh,vertices,faces,1 if level == 2 and material == 0 else material)


def fractured_ground(mesh, high):
    """Low cleaved outcrop blending into ash, contained within a rotation-safe disk."""
    # A tapered skirt ends below the substrate; no square tile boundary is drawn.
    count = 13 if high else 7
    points = outline(71, 13, count)
    vertices = [(x, y, -1.) for x, y in points]
    vertices += [(x*.78+4, y*.78-3, 2.0+1.3*math.sin(i*1.7))
                 for i,(x,y) in enumerate(points)]
    vertices.append((7.,-8.,3.8))
    faces = [tuple(reversed(range(count)))]
    for i in range(count):
        j=(i+1)%count
        faces += [(i,j,count+j,count+i), (count+i,count+j,count*2)]
    solid(mesh,vertices,faces,0)
    if high:
        rock(mesh, 10, 3.5, 4, 0, (42,-31,0), sides=5)
        rock(mesh, 7, 2.5, 9, 0, (-38,29,0), sides=5)


def bedrock_cell(mesh, high):
    """Broad fractured basalt with a split crown and recessed horizontal strata."""
    count=15 if high else 9
    perimeter=[]
    for i in range(count):
        angle=i*math.tau/count
        radius=91+6*math.sin(i*2.31)+2*math.cos(i*.8)
        perimeter.append((radius*math.cos(angle),radius*math.sin(angle)))
    levels=(0., .22, .43, .49, .73, 1.) if high else (0., .43, .49, 1.)
    vertices=[]
    for layer,fraction in enumerate(levels):
        for i,(x,y) in enumerate(perimeter):
            width=1.-.15*fraction + (.04 if .45 < fraction < .6 else 0)
            # Deep diagonal cleaves make each crown read as broken strata,
            # rather than the lid of a square extrusion.
            split=36*math.sin(i*1.7)+19*math.cos(i*.9)
            z=0 if layer == 0 else fraction*(170+split)
            vertices.append((x*width+fraction*6,y*width-fraction*4,z))
    top=(len(levels)-1)*count
    vertices.append((12.,-17.,193.))
    faces=[tuple(reversed(range(count)))]
    faces += [(top+i,top+(i+1)%count,len(vertices)-1) for i in range(count)]
    solid(mesh,vertices,faces,0)
    for layer in range(len(levels)-1):
        faces=[(layer*count+i,layer*count+(i+1)%count,(layer+1)*count+(i+1)%count,(layer+1)*count+i) for i in range(count)]
        solid(mesh,vertices,faces,1 if .4 < levels[layer] < .45 else 0)


def sun_crust(mesh, high):
    """A closed spherical crust split into irregular plates, with narrow hot seams."""
    rows, columns = (9,18) if high else (7,14)
    def point(row, column):
        theta = math.tau*(column%columns)/columns
        latitude = -math.pi/2 + math.pi*row/rows
        if row not in (0,rows):
            latitude += .065*math.sin(column*2.17+row*.93)
            theta += .055*math.sin(column*1.37+row*2.41)
        return (math.cos(latitude)*math.cos(theta), math.cos(latitude)*math.sin(theta), math.sin(latitude))
    for row in range(rows):
        for col in range(columns):
            corners = [point(row,col),point(row,col+1),point(row+1,col+1),point(row+1,col)]
            if row == 0: corners = [corners[0],corners[2],corners[3]]
            if row == rows-1: corners = corners[:3]
            center = tuple(sum(p[j] for p in corners)/len(corners) for j in range(3))
            ring = [tuple(.79*p[j]+.21*center[j] for j in range(3)) for p in corners]
            ring = [tuple(v/math.sqrt(sum(x*x for x in p)) for v in p) for p in ring]
            # Rare displaced fragments preserve a visibly fractured outline.
            lift = 85 if (row*columns+col)%23 == 3 else 0
            vertices = [tuple(v*(r+lift) for v in p) for r in (1190.,1240.) for p in ring]
            n = len(ring)
            faces = [tuple(reversed(range(n))),tuple(range(n,2*n))]
            faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
            solid(mesh,vertices,faces,1)
    for i in range(19 if high else 11):
        theta = i*2.399
        r = 1590+(i*157)%690
        rock(mesh, 85+(i*31)%95, 225+(i*57)%230, i+41, 1,
             (r*math.cos(theta),r*math.sin(theta),490*math.sin(i*1.7)-160),sides=6)


def radial_paving(mesh, high):
    """Concentric fitted stone blocks surrounding the Future Well footprint."""
    import unreal
    for row in range(3):
        inner, outer = 235+row*80, 312+row*80
        count = (32 if high else 24)+row*4
        for i in range(count):
            start = (i+.04+row*.5)*math.tau/count
            end = (i+.96+row*.5)*math.tau/count
            points = [(r*math.cos(a),r*math.sin(a)) for r,a in
                      ((inner,start),(outer,start),(outer,end),(inner,end))]
            mesh.append_simple_extrude_polygon(
                unreal.GeometryScriptPrimitiveOptions(material_id=2 if (i+row)%7 else 0),
                unreal.Transform(location=unreal.Vector(0,0,52)),
                [unreal.Vector2D(*p) for p in points], 6, 0, True,
                unreal.GeometryScriptPrimitiveOriginMode.BASE)


def walk_surface(mesh, high):
    """Tile-sized authored substrate. Relief is supplied by the biome ground kit."""
    import unreal
    mesh.append_simple_extrude_polygon(
        unreal.GeometryScriptPrimitiveOptions(material_id=0),
        unreal.Transform(location=unreal.Vector(0,0,-2)),
        [unreal.Vector2D(x,y) for x,y in ((-100,-100),(100,-100),(100,100),(-100,100))],
        2,0,True,unreal.GeometryScriptPrimitiveOriginMode.BASE)


def outline(radius: float, seed: int, count: int = 11):
    """Counter-clockwise, non-self-intersecting fractured stone outline."""
    return tuple(
        (math.cos(i * math.tau / count) * radius * (0.88 + 0.10 * math.sin(i * 2.31 + seed)),
         math.sin(i * math.tau / count) * radius * (0.87 + 0.11 * math.cos(i * 1.73 + seed)))
        for i in range(count)
    )


def build(mesh, high: bool, kind: str, ground: bool = False):
    import unreal

    def slab(points, z, depth, material=0, x=0., y=0., yaw=0.):
        mesh.append_simple_extrude_polygon(
            unreal.GeometryScriptPrimitiveOptions(material_id=material),
            unreal.Transform(location=unreal.Vector(x, y, z), rotation=unreal.Rotator(0., yaw, 0.)),
            [unreal.Vector2D(*p) for p in points], depth, 0, True,
            unreal.GeometryScriptPrimitiveOriginMode.BASE)

    def rect(w, d, z, h, material=0, x=0., y=0., yaw=0.):
        slab(((-w/2, -d/2), (w/2, -d/2), (w/2, d/2), (-w/2, d/2)), z, h, material, x, y, yaw)

    if ground:
        # Overlapping, shallow relief: no raised obstruction on a legal route.
        if kind == "Shivergrass":
            for i in range(64 if high else 24):
                x, y = 75 * math.sin(i * 2.399), 75 * math.cos(i * 1.719)
                # Bent double-sided ribbons expose a pale face at tactical pitch;
                # low relief preserves route truth. Wind direction is coherent.
                angle = .45 + .35 * math.sin(i * 1.17)
                vertices = []
                for z, length, half_width in ((1,0,2.2),(12,8,1.8),(19,20,.15)):
                    for side in (-1,1):
                        vertices.append((x + length*math.cos(angle)-side*half_width*math.sin(angle),
                                         y + length*math.sin(angle)+side*half_width*math.cos(angle),z))
                faces = [(2,3,1,0),(4,5,3,2)]
                solid(mesh, vertices, faces, 2)
        elif kind in ("Civic", "Solar", "Choir"):
            for x in (-49., 49.):
                for y in (-49., 49.):
                    rect(96, 96, 0, 3, 0, x, y)
            # Incised engineering or accord geometry, restrained warm edge.
            rect(176, 1.1, 3, .5, 2, 0, -87)
            rect(1.1, 176, 3, .5, 2, -87, 0)
            if kind == "Choir":
                rect(52, 1.2, 4, .5, 3, 16, 15, 25)
        else:
            fractured_ground(mesh, high)
        return

    if kind in ("Basalt", "Shivergrass", "Cavern"):
        bedrock_cell(mesh, high)
        if kind == "Cavern":
            for i in range(5 if high else 3):
                x, y = 49 * math.sin(i * 2.4), 49 * math.cos(i * 2.4)
                slab(outline(16, i, 6), 160, 40 + 12 * i, 2, x, y)
                slab(outline(7, i, 6), 170, 22 + 12 * i, 3, x, y)
        elif kind == "Shivergrass":
            slab(outline(27, 10, 7), 182, 18, 2)
    elif kind == "Civic":
        # Buttressed, recessed civic facade; ceramic ribs expose load paths.
        rect(172, 148, 0, 20, 1)
        rect(144, 120, 20, 212, 1)
        for x in (-66., -38., 38., 66.):
            rect(12, 130, 20, 218, 2, x)
        for z in (30., 112., 210.):
            rect(146, 136, z, 10, 0)
        for x in (-18., 18.):
            rect(8, 2, 48, 58, 3, x, -61)
        rect(152, 140, 234, 14, 2)
    elif kind == "Choir":
        # Repeated near-identical frames with an intentionally displaced seam.
        for index in range(3):
            shift = index * 7.
            y = -46 + index * 42
            rect(12, 16, 0, 186, 0, -64 + shift, y)
            rect(12, 16, 0, 186, 0, 64 + shift, y)
            rect(140, 16, 176, 14, 2, shift, y)
            rect(2, 2, 18, 140, 3, -62 + shift, y-9)
    elif kind == "Solar":
        for i in range(4):
            slab(outline(97-i*13, 0, 8), i*18, 19, 1 if i%2 else 0)
        for i in range(4):
            a = i * math.pi / 2 + math.pi / 4
            x, y = 49 * math.cos(a), 49 * math.sin(a)
            rect(16, 20, 72, 112, 0, x, y, i*90)
            rect(3, 3, 88, 70, 2, x, y-11, i*90)
        slab(outline(24, 0, 8), 78, 88, 2)
        slab(outline(8, 0, 6), 166, 20, 3)


def shelf(mesh, high):
    """780 cm bank, flush top and fractured strata below the walk plane."""
    import unreal
    def slab(points, z, h, mat):
        mesh.append_simple_extrude_polygon(
            unreal.GeometryScriptPrimitiveOptions(material_id=mat),
            unreal.Transform(location=unreal.Vector(0., 0., z)),
            [unreal.Vector2D(*p) for p in points], h, 0, True,
            unreal.GeometryScriptPrimitiveOriginMode.BASE)
    # Continuous walk plane. Relief lies below its top, eliminating raised tile seams.
    slab(((-390,-390),(390,-390),(390,390),(-390,390)), 22, 17, 0)
    # Intersecting fracture wedges remain below the continuous walking surface.
    # Their changing cross-sections avoid smooth vertical extrusion walls.
    count = 9 if high else 5
    for side in range(4):
        for j in range(count):
            along = -300 + (j+.5)*600/count
            angle = side*math.pi/2
            x,y = along*math.cos(angle)-305*math.sin(angle), along*math.sin(angle)+305*math.cos(angle)
            rock(mesh, 73, 350+(j*31+side*47)%35, j+side*19, j%2, (x,y,-380), sides=9 if high else 5)
