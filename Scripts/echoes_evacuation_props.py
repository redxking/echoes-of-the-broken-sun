"""Deterministic M01 evacuation-site mesh recipes. Author: Angelis Pseftis.

These are presentation-only props in centimetres. They explain archive handling and
route maintenance without adding a selectable actor, collision, path, or power state.
"""
from __future__ import annotations

import math

REVISION = "m01-evacuation-props-v13"
KINDS = ("ArchiveCradle", "ArchiveFrame", "RoutePaving", "ServiceConduit", "ArchiveApron", "ArchiveLoadingFace")


def solid(mesh, vertices, faces, material=0):
    """Closed faceted solid with authored per-face normals and world-scale UVs."""
    import unreal
    points, normals, uvs, triangles = [], [], [], []
    for face in faces:
        a, b, c = (vertices[index] for index in face[:3])
        cross = ((b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]),
                 (b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]),
                 (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))
        length = math.sqrt(sum(component*component for component in cross))
        if length < 1e-8:
            raise ValueError("Degenerate evacuation-prop face")
        start = len(points)
        for index in face:
            x, y, z = vertices[index]
            points.append(unreal.Vector(x, y, z))
            normals.append(unreal.Vector(*(component/length for component in cross)))
            # Dominant-axis projection preserves UV area on vertical faces.
            axis = max(range(3), key=lambda index: abs(cross[index]))
            uv = (y, z) if axis == 0 else (x, z) if axis == 1 else (x, y)
            uvs.append(unreal.Vector2D(uv[0]/200., uv[1]/200.))
        for index in range(1, len(face)-1):
            triangles.append(unreal.IntVector(start, start+index+1, start+index))
    mesh.append_buffers_to_mesh(unreal.GeometryScriptSimpleMeshBuffers(
        vertices=points, normals=normals, uv0=uvs, triangles=triangles), material_id=material)


def chamfered_prism(mesh, width, depth, z, height, material, x=0., y=0., cut=0.):
    """One shaped extrusion; chamfers keep equipment from reading as piled cubes."""
    cut = min(cut or min(width, depth)*.18, width*.35, depth*.35)
    outline = ((-width/2+cut, -depth/2), (width/2-cut, -depth/2),
               (width/2, -depth/2+cut), (width/2, depth/2-cut),
               (width/2-cut, depth/2), (-width/2+cut, depth/2),
               (-width/2, depth/2-cut), (-width/2, -depth/2+cut))
    vertices = [(x+px, y+py, z) for px, py in outline]
    vertices += [(px, py, z+height) for px, py, _ in vertices]
    count = len(outline)
    faces = [tuple(reversed(range(count))), tuple(range(count, count*2))]
    faces += [(index, (index+1) % count, (index+1) % count+count, index+count)
              for index in range(count)]
    solid(mesh, vertices, faces, material)


def angled_beam(mesh, length, width, z, height, material, x, y, angle=0.):
    """A chamfered structural member in the local XY plane."""
    # Construct the prism locally then rotate its profile for deterministic framing members.
    cosine, sine = math.cos(angle), math.sin(angle)
    cut = min(length, width)*.18
    outline = ((-length/2+cut, -width/2), (length/2-cut, -width/2),
               (length/2, -width/2+cut), (length/2, width/2-cut),
               (length/2-cut, width/2), (-length/2+cut, width/2),
               (-length/2, width/2-cut), (-length/2, -width/2+cut))
    base = [(x+px*cosine-py*sine, y+px*sine+py*cosine, z) for px, py in outline]
    vertices = base + [(px, py, z+height) for px, py, _ in base]
    count = len(outline)
    faces = [tuple(reversed(range(count))), tuple(range(count, count*2))]
    faces += [(index, (index+1) % count, (index+1) % count+count, index+count)
              for index in range(count)]
    solid(mesh, vertices, faces, material)


def tapered_housing(mesh, width, depth, z, height, material, x=0., y=0., cut=0., top_scale=.78):
    """A chamfered ceramic equipment shell with an inset upper shoulder."""
    cut = min(cut or min(width, depth)*.16, width*.30, depth*.30)
    outline = ((-width/2+cut, -depth/2), (width/2-cut, -depth/2),
               (width/2, -depth/2+cut), (width/2, depth/2-cut),
               (width/2-cut, depth/2), (-width/2+cut, depth/2),
               (-width/2, depth/2-cut), (-width/2, -depth/2+cut))
    base = [(x+px, y+py, z) for px, py in outline]
    cap = [(x+px*top_scale, y+py*top_scale, z+height) for px, py in outline]
    count = len(outline)
    faces = [tuple(reversed(range(count))), tuple(range(count, count*2))]
    faces += [(index, (index+1) % count, (index+1) % count+count, index+count)
              for index in range(count)]
    solid(mesh, base + cap, faces, material)


def tube_x(mesh, length, radius, x, y, z, material, sides=8):
    """A capped octagonal service tube along X, kept clearly separate from weapons."""
    ring = [(0., math.cos(index*math.tau/sides)*radius,
             math.sin(index*math.tau/sides)*radius) for index in range(sides)]
    vertices = [(x-length*.5+px, y+py, z+pz) for px, py, pz in ring]
    vertices += [(x+length*.5+px, y+py, z+pz) for px, py, pz in ring]
    faces = [tuple(reversed(range(sides))), tuple(range(sides, sides*2))]
    faces += [(index, (index+1) % sides, (index+1) % sides+sides, index+sides)
              for index in range(sides)]
    solid(mesh, vertices, faces, material)


def archive_cradle(mesh, high):
    """Low carrier cradle with load rails, tapered archive housings and fitted restraints."""
    for y in (-44., 44.):
        angled_beam(mesh, 158, 13, 7, 13, 1, 0, y)
        angled_beam(mesh, 148, 5, 20, 4, 0, 0, y)
        for x in (-61., 0., 61.) if high else (-61., 61.):
            chamfered_prism(mesh, 25, 22, 0, 13, 1, x, y, 5)
    for x in (-43., 0., 43.) if high else (-43., 43.):
        # Grounded transverse saddles connect the cargo shell to both load rails.
        chamfered_prism(mesh, 42, 96, 0, 22, 1, x, 0, 5)
        tapered_housing(mesh, 34, 60, 22, 43, 0, x, 0, 8, .72)
        chamfered_prism(mesh, 21, 46, 30, 27, 2, x, 0, 5)
        # One continuous retaining band per shell returns down to the load rails.
        angled_beam(mesh, 9, 96, 64, 5, 1, x, 0)
        for y in (-44., 44.):
            chamfered_prism(mesh, 9, 8, 20, 49, 1, x, y, 1.5)
        if high:
            chamfered_prism(mesh, 14, 10, 45, 11, 1, x-13, -27, 2)
    # A low, accessible inspection cabinet and its non-emissive keyed fitting.
    chamfered_prism(mesh, 48, 30, 0, 17, 2, 0, -68, 6)
    tapered_housing(mesh, 42, 24, 17, 19, 2, 0, -68, 6, .70)
    chamfered_prism(mesh, 13, 8, 36, 14, 1, 0, -80, 2)
    chamfered_prism(mesh, 7, 7, 41, 8, 3, 0, -85, 1)


def archive_service_frame(mesh, high):
    """An occupied loading frame: restrained equipment makes its blocked footprint legible."""
    for x in (-70., 70.):
        tapered_housing(mesh, 34, 48, 0, 16, 2, x, 0, 8, .80)
        tapered_housing(mesh, 21, 20, 16, 204, 1, x, 0, 4, .88)
        chamfered_prism(mesh, 48, 18, 44, 13, 0, x, 0, 4)
        if high:
            angled_beam(mesh, 56, 10, 67, 9, 1, x, 0, math.pi*.5)
    angled_beam(mesh, 170, 22, 220, 20, 1, 0, 0)
    angled_beam(mesh, 158, 8, 240, 7, 2, 0, 0)
    # A retained equipment load occupies the blocked bay; the frame must not
    # advertise a walk-through gap that the authoritative terrain cannot grant.
    chamfered_prism(mesh, 148, 112, 0, 12, 1, 0, -10, 10)
    tapered_housing(mesh, 108, 84, 12, 92, 0, 0, -10, 13, .84)
    for x in (-31., 31.):
        chamfered_prism(mesh, 8, 82, 96, 10, 1, x, -10, 2)
    # Trolley is parked at the east-side end, with its load line attached
    # continuously from the carriage to the handling fixture below.
    chamfered_prism(mesh, 4, 4, 109, 122, 1, 42, 22, .6)
    tapered_housing(mesh, 42, 34, 221, 35, 0, 42, 0, 7, .76)
    chamfered_prism(mesh, 20, 44, 231, 12, 1, 42, 0, 4)
    tube_x(mesh, 9, 3.5, 42, 22, 136, 1, 6)
    chamfered_prism(mesh, 23, 13, 96, 13, 1, 42, 22, 4)
    chamfered_prism(mesh, 11, 11, 79, 14, 3, 42, 22, 2)


def route_paving(mesh, high):
    """One flush service slab with continuous travel wear, without a small-tile grid."""
    chamfered_prism(mesh, 198, 198, 0, 3, 2, 0, 0, 10 if high else 6)
    # Paired pale wear tracks continue across panel joints along the working route.
    for y in (-64., 64.):
        angled_beam(mesh, 196, 3, 3, .35, 0, 0, y)
    if high:
        # One fitted maintenance repair earns the close detail; no dotted fastener grid.
        chamfered_prism(mesh, 30, 44, 3, .30, 0, 32, 18, 5)


def service_conduit(mesh, high):
    """Low service raceway with real conduit tubes, brackets and terminated fittings."""
    tapered_housing(mesh, 126, 38, 0, 18, 2, 0, 0, 8, .86)
    # Keep capped west ends; bury east ends inside the tapered receiving fittings.
    tube_x(mesh, 128, 7, 8, -10, 33, 1)
    tube_x(mesh, 120, 5, 8, 10, 48, 1)
    for x in (-47., 0., 47.) if high else (-47., 47.):
        chamfered_prism(mesh, 18, 50, 17, 11, 0, x, 0, 4)
        chamfered_prism(mesh, 15, 38, 28, 46, 1, x, 0, 4)
        angled_beam(mesh, 45, 8, 72, 8, 2, x, 0)
        chamfered_prism(mesh, 14, 14, 79, 12, 3, x, -18, 3)
    for x in (-64., 64.):
        tapered_housing(mesh, 21, 48, 0, 25, 2, x, 0, 5, .78)
        chamfered_prism(mesh, 11, 15, 26, 13, 1, x, -13, 2)
    # Short capped termination boxes read as disconnected civic service, never a turret.
    tapered_housing(mesh, 26, 23, 30, 18, 0, 72, -10, 4, .72)
    tapered_housing(mesh, 22, 19, 45, 16, 0, 68, 10, 4, .72)


def archive_apron(mesh, high):
    """Flush five-by-four civic loading apron, centred for its registered pivot.

    The landmark compiler supplies the (-100 cm Y) placement offset.  Keeping this
    source mesh at its geometric centre prevents that source-of-truth offset from
    being baked twice into the recovery apron.
    """
    columns, rows = 5, (4 if high else 2)
    panel_width = 192.
    panel_depth = (192. if high else 392.)
    x_centres = tuple(-400. + 200.*index for index in range(columns))
    y_centres = tuple(-300. + 200.*index for index in range(rows)) if high else (-200., 200.)
    for x in x_centres:
        for y in y_centres:
            chamfered_prism(mesh, panel_width, panel_depth, 0, 2.9, 0, x, y, 10 if high else 7)

    # Recessed perimeter and inter-panel seams frame the work area without a lip.
    for x in (-497., -200., 0., 200., 497.):
        angled_beam(mesh, 4, 790, 2.35, .5, 2, x, 0)
    for y in (-397., -200., 0., 200., 397.) if high else (-397., 0., 397.):
        angled_beam(mesh, 990, 4, 2.35, .5, 2, 0, y)

    # Surface inlays sit just above the 2.9 cm panels, within the flush-floor
    # envelope. Earlier markings were buried underneath the panel tops.
    rails = (-300., -100., 100., 300.)
    for x in rails:
        angled_beam(mesh, 10, 730, 2.94, .06, 1, x, 0)
    registrations = ((-300., -220.), (-100., -220.), (100., -220.), (300., -220.),
                     (-300., 160.), (-100., 160.), (100., 160.), (300., 160.))
    for x, y in registrations:
        chamfered_prism(mesh, 104, 42, 2.94, .06, 1, x, y, 5)

    # Two visibly fitted repairs and their muted, non-powered fastening plates
    # record repeated civic use without adding letters, hazards, or clutter.
    repairs = ((-105., 102.), (292., -96.))
    for x, y in repairs:
        chamfered_prism(mesh, 86, 72, 2.94, .10, 2, x, y, 8)
        if high:
            chamfered_prism(mesh, 58, 5, 3.05, .25, 3, x, y-21., 2)
            chamfered_prism(mesh, 5, 44, 3.05, .25, 3, x-25., y, 2)


def archive_loading_face(mesh, high):
    """Five blocked tiles of continuous, local-Y-facing archive handling."""
    # This single retaining mass closes the whole blocked span so its visually
    # occupied treatment cannot imply an unwalkable gap as a false passage.
    tapered_housing(mesh, 998, 198, 0, 42, 2, 0, 0, 12, .94)
    chamfered_prism(mesh, 972, 156, 0, 13, 1, 0, 0, 12)
    chamfered_prism(mesh, 956, 12, 42, 8, 0, 0, -70, 4)

    cassette_x = (-330., -110., 110., 330.)
    for x in cassette_x:
        tapered_housing(mesh, 158, 98, 42, 91, 0, x, 10, 13, .78)
        angled_beam(mesh, 12, 112, 102, 8, 1, x-35, 10)
        angled_beam(mesh, 12, 112, 102, 8, 1, x+35, 10)
        chamfered_prism(mesh, 18, 10, 118, 10, 3, x, -42, 3)
        if high:
            chamfered_prism(mesh, 86, 5, 68, 6, 2, x, -32, 2)

    # Rear piers stand behind the cassettes; crossheads carry the front rail.
    # Neither posts nor load paths pass through the retained archive housings.
    for x in (-330., 0., 330.):
        tapered_housing(mesh, 58, 68, 0, 39, 2, x, 62, 10, .82)
        tapered_housing(mesh, 31, 31, 39, 287, 1, x, 78, 5, .90)
        angled_beam(mesh, 115, 22, 314, 14, 1, x, 35, math.pi*.5)
        chamfered_prism(mesh, 48, 18, 292, 14, 0, x, 78, 4)

    angled_beam(mesh, 920, 34, 326, 20, 1, 0, 0)
    angled_beam(mesh, 908, 8, 346, 7, 0, 0, 0)
    # Off-centre parked trolley, cable, and fixture form one connected chain.
    tapered_housing(mesh, 52, 44, 319, 27, 0, 165, 0, 7, .76)
    chamfered_prism(mesh, 24, 52, 327, 10, 1, 165, 0, 4)
    chamfered_prism(mesh, 5, 5, 139, 180, 1, 165, 0, 1)
    chamfered_prism(mesh, 33, 20, 126, 14, 1, 165, 0, 5)
    chamfered_prism(mesh, 14, 12, 120, 9, 3, 165, -12, 3)


def build(mesh, high: bool, kind: str):
    if kind not in KINDS:
        raise ValueError("Unknown evacuation prop kind: %s" % kind)
    {"ArchiveCradle": archive_cradle,
     "ArchiveFrame": archive_service_frame,
     "RoutePaving": route_paving,
     "ServiceConduit": service_conduit,
     "ArchiveApron": archive_apron,
     "ArchiveLoadingFace": archive_loading_face}[kind](mesh, high)
