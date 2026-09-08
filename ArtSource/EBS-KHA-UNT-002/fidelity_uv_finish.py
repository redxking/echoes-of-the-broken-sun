"""Create a shared, welded UV0 atlas for the Riftstalker fidelity pilot.

Run this file with Blender, not CPython.  It deliberately operates on the JSON
review exports rather than changing the source generator or GLB writer.  The
result is an override file per state/LOD; the production exporter is expected
to apply those corner UVs while retaining its existing skin, sockets and
COLOR_0 attributes.

The input JSON is unindexed because it mirrors the deterministic exporter.
Vertices are welded only when position, component and COLOR_0 agree.  This
keeps hard component boundaries and state membership intact while allowing
Blender's UV tools to see each connected solid as topology rather than a pile
of unrelated triangles.

``--bake-transfer`` is intentionally opt-in.  It re-emits colour/mask maps by
sampling the named source UV layer while baking to the generated target layer.
The normal-map transfer samples the old DirectX normal through its explicit
source UV layer, flips its green channel for Blender's OpenGL normal-map node,
then bakes the shader normal into the target tangent basis with DirectX green
output.  Copying old tangent RGB values into a changed UV layout would be
invalid.

Author: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import sys
from collections import defaultdict

import bpy
# Blender does not add the executed script's directory to sys.path on macOS.
_SCRIPT_DIR = str(pathlib.Path(__file__).resolve().parent)
if _SCRIPT_DIR not in sys.path:
    sys.path.insert(0, _SCRIPT_DIR)
import test_fidelity_uv as uv_qa

AUTHOR = "Angelis Pseftis"
ASSET = "EBS-KHA-UNT-002"
SOURCE_PREFIX = "T_EBS_KHA_UNT_002_"
EPS = 1.0e-5

# The deterministic JSON/GLB path records texture coordinates against image
# rows (top origin, V down). Blender's mesh UV data uses bottom origin, V up.
# Keep both transforms named so transfer and re-export cannot silently drift.
JSON_UV_CONVENTION = "top_left_image_origin_v_down"
BLENDER_UV_CONVENTION = "bottom_left_uv_origin_v_up"


def json_uv_to_blender(value):
    return (float(value[0]), 1.0 - float(value[1]))


def blender_uv_to_json(value):
    return (float(value[0]), 1.0 - float(value[1]))


def _q(value: float, scale: float = 100000.0) -> int:
    return int(round(value * scale))


def _distance(a, b) -> int:
    """A rotation/translation invariant edge length, quantized in centimetres."""
    return _q(math.sqrt(sum((a[i] - b[i]) ** 2 for i in range(3))), 10000.0)


class Solid:
    """One source component after a component-local, channel-aware weld."""

    def __init__(self, source_name: str, component: str, data: dict, face_rows: list[int]):
        self.source_name = source_name
        self.component = component
        self.data = data
        self.face_rows = face_rows
        self.vertices: list[tuple[float, float, float]] = []
        self.faces: list[list[int]] = []
        self.corner_uv: list[list[tuple[float, float]]] = []
        self.corner_old_uv: list[list[tuple[float, float]]] = []
        weld: dict[tuple, int] = {}
        for face_row in face_rows:
            face = data["faces"][face_row]
            local_face = []
            old_uv = []
            for old_index in face["vertices"]:
                point = data["vertices"][old_index]
                color = data["colors"][old_index]
                key = (
                    tuple(_q(float(v)) for v in point),
                    tuple(_q(float(v)) for v in color),
                )
                local_index = weld.get(key)
                if local_index is None:
                    local_index = len(self.vertices)
                    weld[key] = local_index
                    self.vertices.append(tuple(float(v) for v in point))
                local_face.append(local_index)
                old_uv.append(tuple(float(v) for v in data["uv"][old_index]))
            self.faces.append(local_face)
            self.corner_old_uv.append(old_uv)
            self.corner_uv.append([])
        self.color_signature = tuple(sorted({tuple(_q(float(v)) for v in data["colors"][i])
                                             for row in face_rows
                                             for i in data["faces"][row]["vertices"]}))
        # Geometry copies are permitted to share one atlas placement only when
        # they also sample precisely the same source pixels.  An equal edge
        # signature does not establish equal paint, normal, MRE or state-mask
        # semantics: their source chart locations are part of the identity.
        self.texture_signature = tuple(
            (int(data["faces"][row]["material"]),
             tuple((round(u, 8), round(v, 8)) for u, v in old_uv))
            for row, old_uv in zip(face_rows, self.corner_old_uv)
        )

    def fingerprint(self) -> tuple:
        """Strictly enough shape data to share an atlas placement safely.

        The flattened generator emits mirrored and translated copies of several
        solids.  Edge/degree signatures are transform invariant, while the
        follow-up ``compatible_with`` check prevents a hash collision from
        sharing UVs.
        """
        incidence = defaultdict(list)
        face_sigs = []
        for face in self.faces:
            lengths = [_distance(self.vertices[face[i]], self.vertices[face[(i + 1) % len(face)]])
                       for i in range(len(face))]
            face_sigs.append((len(face), tuple(sorted(lengths))))
            for i in range(len(face)):
                a, b = sorted((face[i], face[(i + 1) % len(face)]))
                length = _distance(self.vertices[a], self.vertices[b])
                incidence[a].append(length); incidence[b].append(length)
        degree_sigs = tuple(sorted((len(edges), tuple(sorted(edges))) for edges in incidence.values()))
        return (len(self.vertices), len(self.faces), tuple(sorted(face_sigs)), degree_sigs,
                self.color_signature, self.texture_signature)

    def compatible_with(self, other: "Solid") -> bool:
        if len(self.faces) != len(other.faces) or len(self.vertices) != len(other.vertices):
            return False
        if self.color_signature != other.color_signature:
            return False
        if self.texture_signature != other.texture_signature:
            return False
        # The generator preserves polygon/corner order across true copies.  Do
        # not treat similarly sized but differently constructed solids as copies.
        for a, b in zip(self.faces, other.faces):
            if len(a) != len(b):
                return False
            da = sorted(_distance(self.vertices[a[i]], self.vertices[a[(i + 1) % len(a)]]) for i in range(len(a)))
            db = sorted(_distance(other.vertices[b[i]], other.vertices[b[(i + 1) % len(b)]]) for i in range(len(b)))
            if da != db:
                return False
        return True


def _load_solids(root: pathlib.Path):
    sources = []
    solids = []
    for path in sorted(root.glob("*_lod[01].json")):
        data = json.loads(path.read_text())
        required = {"vertices", "faces", "components", "uv", "colors"}
        missing = required - set(data)
        if missing:
            raise ValueError(f"{path}: missing {sorted(missing)}")
        if len(data["faces"]) != len(data["components"]):
            raise ValueError(f"{path}: face/component count mismatch")
        by_component = defaultdict(list)
        for i, component in enumerate(data["components"]):
            by_component[component].append(i)
        source = {"path": path, "name": path.name, "data": data,
                  "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
        sources.append(source)
        for component, rows in sorted(by_component.items()):
            solids.append(Solid(path.name, component, data, rows))
    if not sources:
        raise ValueError(f"No *_lod[01].json meshes found in {root}")
    return sources, solids


def _group_equivalent_solids(solids: list[Solid]):
    buckets: dict[tuple, list[list[Solid]]] = defaultdict(list)
    for solid in solids:
        signature = solid.fingerprint()
        for group in buckets[signature]:
            if solid.compatible_with(group[0]):
                group.append(solid)
                break
        else:
            buckets[signature].append([solid])
    return [group for groups in buckets.values() for group in groups]


def _make_master(groups: list[list[Solid]]):
    """Build one disconnected representative of each reusable geometric solid."""
    verts = []
    faces = []
    old_uv = []
    references = []
    offset = 0
    for group_index, group in enumerate(groups):
        solid = group[0]
        verts.extend([(x / 100.0, y / 100.0, z / 100.0) for x, y, z in solid.vertices])
        for local_face_index, face in enumerate(solid.faces):
            faces.append([offset + index for index in face])
            old_uv.append(solid.corner_old_uv[local_face_index])
            references.append((group_index, local_face_index))
        offset += len(solid.vertices)
    mesh = bpy.data.meshes.new("Riftstalker_UV_Representatives")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    source_uv = mesh.uv_layers.new(name="UV_Source")
    for poly, old in zip(mesh.polygons, old_uv):
        for loop_index, value in zip(poly.loop_indices, old):
            source_uv.data[loop_index].uv = json_uv_to_blender(value)
    target_uv = mesh.uv_layers.new(name="UV_Target")
    obj = bpy.data.objects.new("Riftstalker_UV_Representatives", mesh)
    bpy.context.collection.objects.link(obj)
    for poly in mesh.polygons:
        poly.use_smooth = False
    return obj, source_uv, target_uv, references


def _mark_spanning_tree_seams(mesh) -> int:
    """Open each disconnected solid along a face-adjacency spanning tree.

    The pilot's closed, all-triangle mineral shells cannot be solved by an
    automatic unwrap without a cut.  Keeping a maximum-size face tree open is
    a conservative explicit seam policy: it creates one chart per connected
    solid rather than one per faceted triangle, while preserving every source
    polygon and flat normal.
    """
    edge_faces = defaultdict(list)
    for poly in mesh.polygons:
        for loop in poly.loop_indices:
            edge_faces[mesh.loops[loop].edge_index].append(poly.index)
    graph = defaultdict(list)
    for edge, faces in edge_faces.items():
        if len(faces) == 2:
            a, b = faces
            graph[a].append((b, edge)); graph[b].append((a, edge))
    keep = set(); visited = set()
    for start in range(len(mesh.polygons)):
        if start in visited:
            continue
        visited.add(start); stack = [start]
        while stack:
            face = stack.pop()
            for neighbour, edge in graph[face]:
                if neighbour not in visited:
                    visited.add(neighbour); stack.append(neighbour); keep.add(edge)
    seam_count = 0
    for edge, faces in edge_faces.items():
        is_seam = len(faces) == 2 and edge not in keep
        mesh.edges[edge].use_seam = is_seam
        seam_count += int(is_seam)
    return seam_count


def _unwrap_and_pack(obj, target_uv, angle_degrees: float, margin: float, mode: str):
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    obj.data.uv_layers.active = target_uv
    seam_count = 0
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    # The face-tree trial had a compact atlas but folded non-developable
    # mineral surfaces over themselves. Smart Project is Blender's conservative
    # non-overlap production path. The tree path is retained only for evidence.
    if mode == "smart":
        # Smart Project's own margin is normalized during its internal pack.
        # Keep it at zero; the following explicit FRACTION pack owns spacing.
        bpy.ops.uv.smart_project(angle_limit=math.radians(angle_degrees), island_margin=0.0,
                                 area_weight=0.0, correct_aspect=True, scale_to_bounds=True)
    elif mode == "tree":
        bpy.ops.object.mode_set(mode="OBJECT")
        seam_count = _mark_spanning_tree_seams(obj.data)
        obj.data.update()
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.unwrap(method="ANGLE_BASED", fill_holes=True, correct_aspect=True,
                          use_subsurf_data=False, margin=margin)
    else:
        raise ValueError(f"unknown unwrap mode {mode}")
    # Normalize texel density from real surface area, then pack with a physical
    # atlas fraction (four texels at 2048 by default), not a relative 1% gap.
    bpy.ops.uv.average_islands_scale()
    bpy.ops.uv.pack_islands(rotate=True, rotate_method="AXIS_ALIGNED_Y", scale=True,
                            margin_method="FRACTION", margin=margin)
    bpy.ops.object.mode_set(mode="OBJECT")
    # Blender may invalidate a UV-layer RNA handle during an unwrap operator.
    # Return the live active layer rather than retaining the pre-operator view.
    return obj.data.uv_layers.active, seam_count


def _island_count(mesh, uv_layer) -> int:
    """Count UV-connected polygon islands after Smart Project."""
    edge_uses = defaultdict(list)
    for poly in mesh.polygons:
        loops = list(poly.loop_indices)
        for here, nxt in zip(loops, loops[1:] + loops[:1]):
            a, b = mesh.loops[here].vertex_index, mesh.loops[nxt].vertex_index
            values = {a: tuple(round(x, 7) for x in uv_layer.data[here].uv),
                      b: tuple(round(x, 7) for x in uv_layer.data[nxt].uv)}
            edge_uses[tuple(sorted((a, b)))].append((poly.index, values))
    adjacent = defaultdict(set)
    for (a, b), uses in edge_uses.items():
        if len(uses) != 2:
            continue
        (left, uv_left), (right, uv_right) = uses
        if uv_left[a] == uv_right[a] and uv_left[b] == uv_right[b]:
            adjacent[left].add(right); adjacent[right].add(left)
    remaining = set(range(len(mesh.polygons))); islands = 0
    while remaining:
        islands += 1; stack = [remaining.pop()]
        while stack:
            current = stack.pop()
            for neighbour in adjacent[current]:
                if neighbour in remaining:
                    remaining.remove(neighbour); stack.append(neighbour)
    return islands


def _atlas_metrics(mesh, uv_layer, size: int):
    """Surface/UV occupancy for the unique representative geometry only."""
    surface_cm2 = 0.0; uv_area = 0.0
    for poly in mesh.polygons:
        loops = list(poly.loop_indices)
        for index in range(1, len(loops) - 1):
            first, second, third = (mesh.vertices[mesh.loops[loop].vertex_index].co for loop in (loops[0], loops[index], loops[index + 1]))
            world = .5 * (second - first).cross(third - first).length * 10000.0
            uvs = [uv_layer.data[loop].uv for loop in (loops[0], loops[index], loops[index + 1])]
            atlas = .5 * abs((uvs[1].x-uvs[0].x)*(uvs[2].y-uvs[0].y) - (uvs[1].y-uvs[0].y)*(uvs[2].x-uvs[0].x))
            surface_cm2 += world; uv_area += atlas
    theoretical = math.sqrt(size * size / surface_cm2) if surface_cm2 else 0.0
    actual = math.sqrt(uv_area * size * size / surface_cm2) if surface_cm2 else 0.0
    return {"unique_surface_cm2": surface_cm2, "occupied_uv_fraction": uv_area,
            "theoretical_px_per_cm_without_padding": theoretical,
            "actual_px_per_cm_from_unique_representatives": actual}


def _representative_overlap_pairs(mesh, uv_layer, epsilon: float = 1.0e-12):
    """Return exact positive-area overlaps between different representative polygons.

    This uses the same convex clipping predicate as :mod:`test_fidelity_uv`,
    but against Blender's live mesh before the corner overrides are emitted.
    A shared chart border has zero clipped area and is deliberately excluded.
    """
    triangles = []
    for poly in mesh.polygons:
        loops = list(poly.loop_indices)
        for index in range(1, len(loops) - 1):
            triangle = [tuple(float(v) for v in uv_layer.data[loop].uv)
                        for loop in (loops[0], loops[index], loops[index + 1])]
            triangles.append((min(x for x, y in triangle), max(x for x, y in triangle),
                              min(y for x, y in triangle), max(y for x, y in triangle), triangle, poly.index))
    triangles.sort(key=lambda item: item[0]); active = []; areas = defaultdict(float)
    for triangle in triangles:
        active = [old for old in active if old[1] > triangle[0] + epsilon]
        for old in active:
            if old[3] <= triangle[2] + epsilon or triangle[3] <= old[2] + epsilon:
                continue
            if old[5] == triangle[5]:
                continue
            area = uv_qa.overlap_area(old[4], triangle[4])
            if area > epsilon:
                areas[tuple(sorted((old[5], triangle[5])))] += area
        active.append(triangle)
    return [(area, first, second) for (first, second), area in sorted(areas.items()) if area > epsilon]


def _detach_and_repack_overlaps(obj, target_uv, margin: float, max_passes: int = 4):
    """Cut one polygon from every live positive-overlap pair and repack.

    Moving a selected polygon's existing UV coordinates outside 0..1 first is
    intentional: it breaks its chart identity before Pack Islands sees it.
    Its edge seams and local UV shape are preserved; geometry, corner normals,
    skinning, sockets and COLOR_0 are untouched.
    """
    repairs = []
    for pass_index in range(max_passes):
        pairs = _representative_overlap_pairs(obj.data, target_uv)
        if not pairs:
            return {"passes": pass_index, "detached_polygons": repairs, "remaining_pairs": 0}
        # Greedily detach the higher numbered member.  A single polygon may
        # resolve multiple pairs and is recorded once per pass.
        chosen = sorted({second for _area, _first, second in pairs})
        for serial, polygon_index in enumerate(chosen):
            poly = obj.data.polygons[polygon_index]
            loops = list(poly.loop_indices)
            centre_u = sum(target_uv.data[loop].uv.x for loop in loops) / len(loops)
            centre_v = sum(target_uv.data[loop].uv.y for loop in loops) / len(loops)
            # Preserve the polygon's local shape while ensuring no loop shares
            # its prior chart coordinates during the next pack operation.
            destination = 10.0 + serial * 2.0
            for loop in loops:
                value = target_uv.data[loop].uv
                target_uv.data[loop].uv = (destination + value.x - centre_u,
                                            destination + value.y - centre_v)
                obj.data.edges[obj.data.loops[loop].edge_index].use_seam = True
        repairs.append({"pass": pass_index + 1, "pair_count": len(pairs), "polygons": chosen,
                        "overlap_uv_area": sum(area for area, _first, _second in pairs)})
        obj.data.update()
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.average_islands_scale()
        bpy.ops.uv.pack_islands(rotate=True, rotate_method="AXIS_ALIGNED_Y", scale=True,
                                margin_method="FRACTION", margin=margin)
        bpy.ops.object.mode_set(mode="OBJECT")
        target_uv = obj.data.uv_layers.active
    remaining = _representative_overlap_pairs(obj.data, target_uv)
    if remaining:
        raise RuntimeError(f"UV repair exhausted {max_passes} passes with {len(remaining)} positive-area pairs")
    return {"passes": max_passes, "detached_polygons": repairs, "remaining_pairs": 0}


def _assign_target_uv(groups, obj, target_uv, references):
    by_group_face = {}
    for poly, (group_index, local_face_index) in zip(obj.data.polygons, references):
        by_group_face[(group_index, local_face_index)] = [blender_uv_to_json(target_uv.data[i].uv)
                                                           for i in poly.loop_indices]
    for group_index, group in enumerate(groups):
        for solid in group:
            for face_index in range(len(solid.faces)):
                values = by_group_face[(group_index, face_index)]
                if len(values) != len(solid.faces[face_index]):
                    raise AssertionError("UV corner-count mismatch")
                solid.corner_uv[face_index] = values


def _write_overrides(out: pathlib.Path, sources, solids: list[Solid]):
    by_source = defaultdict(list)
    for solid in solids:
        by_source[solid.source_name].append(solid)
    written = []
    for source in sources:
        data = source["data"]
        uv = [None] * len(data["faces"])
        for solid in by_source[source["name"]]:
            for row, corners in zip(solid.face_rows, solid.corner_uv):
                uv[row] = [[round(a, 8), round(b, 8)] for a, b in corners]
        if any(value is None for value in uv):
            raise AssertionError(f"Missing target UVs for {source['name']}")
        record = {
            "author": AUTHOR,
            "asset": ASSET,
            "status": "UV_OVERRIDE_READY_NOT_YET_APPLIED_TO_GLB",
            "source_json": source["name"],
            "source_sha256": source["sha256"],
            "uv_layer": "UV_Target",
            "source_json_uv_convention": JSON_UV_CONVENTION,
            "blender_working_uv_convention": BLENDER_UV_CONVENTION,
            "polygon_corner_uv_convention": JSON_UV_CONVENTION,
            "polygon_corner_uv": uv,
        }
        path = out / (source["path"].stem + ".uv-override.json")
        path.write_text(json.dumps(record, indent=2) + "\n")
        written.append({"path": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    return written


def _save_png(image, path: pathlib.Path):
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()


def _bake_emit(obj, source_uv, target_uv, source_path: pathlib.Path, target_path: pathlib.Path, size: int, color: bool):
    image_source = bpy.data.images.load(str(source_path), check_existing=True)
    image_source.colorspace_settings.name = "sRGB" if color else "Non-Color"
    image_target = bpy.data.images.new(target_path.stem, width=size, height=size, alpha=False, float_buffer=False)
    image_target.colorspace_settings.name = "sRGB" if color else "Non-Color"
    material = bpy.data.materials.new("UVTransfer_" + target_path.stem)
    material.use_nodes = True
    nodes = material.node_tree.nodes; links = material.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    emit = nodes.new("ShaderNodeEmission")
    sample = nodes.new("ShaderNodeTexImage"); sample.image = image_source
    uv_node = nodes.new("ShaderNodeUVMap"); uv_node.uv_map = source_uv.name
    target = nodes.new("ShaderNodeTexImage"); target.image = image_target; target.select = True
    nodes.active = target
    links.new(uv_node.outputs["UV"], sample.inputs["Vector"])
    links.new(sample.outputs["Color"], emit.inputs["Color"])
    links.new(emit.outputs["Emission"], output.inputs["Surface"])
    obj.data.materials.clear(); obj.data.materials.append(material)
    obj.data.uv_layers.active = target_uv
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.context.scene.render.engine = "CYCLES"
    bpy.ops.object.bake(type="EMIT", margin=16, use_clear=True)
    _save_png(image_target, target_path)
    bpy.data.materials.remove(material, do_unlink=True)
    bpy.data.images.remove(image_target, do_unlink=True)


def _bake_normal(obj, source_uv, target_uv, source_path: pathlib.Path, target_path: pathlib.Path, size: int):
    """Transfer source detail into the new target tangent basis.

    Blender's glTF importer uses ``ShaderNodeNormalMap.uv_map`` for an
    explicit source UV identity.  The same node is used here.  The source map
    is DirectX (negative Y); it is converted to Blender's OpenGL convention
    before reaching Principled Normal.  Cycles then bakes that perturbed normal
    in the target UV tangent basis and writes DirectX (negative Y) output.
    """
    if not source_path.is_file():
        raise FileNotFoundError(source_path)
    source = bpy.data.images.load(str(source_path), check_existing=True)
    source.colorspace_settings.name = "Non-Color"
    target = bpy.data.images.new(target_path.stem, width=size, height=size, alpha=False, float_buffer=False)
    target.colorspace_settings.name = "Non-Color"
    material = bpy.data.materials.new("UVTransfer_RetangentNormal")
    material.use_nodes = True
    nodes = material.node_tree.nodes; links = material.node_tree.links
    principled = nodes.get("Principled BSDF")
    source_tex = nodes.new("ShaderNodeTexImage"); source_tex.image = source
    source_uv_node = nodes.new("ShaderNodeUVMap"); source_uv_node.uv_map = source_uv.name
    split = nodes.new("ShaderNodeSeparateColor")
    invert_green = nodes.new("ShaderNodeMath"); invert_green.operation = "SUBTRACT"
    invert_green.inputs[0].default_value = 1.0
    combine = nodes.new("ShaderNodeCombineColor")
    normal_map = nodes.new("ShaderNodeNormalMap"); normal_map.uv_map = source_uv.name
    target_node = nodes.new("ShaderNodeTexImage"); target_node.image = target; target_node.select = True
    nodes.active = target_node
    links.new(source_uv_node.outputs["UV"], source_tex.inputs["Vector"])
    links.new(source_tex.outputs["Color"], split.inputs["Color"])
    links.new(split.outputs["Red"], combine.inputs["Red"])
    links.new(split.outputs["Green"], invert_green.inputs[1])
    links.new(invert_green.outputs[0], combine.inputs["Green"])
    links.new(split.outputs["Blue"], combine.inputs["Blue"])
    links.new(combine.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])
    obj.data.materials.clear(); obj.data.materials.append(material)
    obj.data.uv_layers.active = target_uv
    bpy.context.view_layer.objects.active = obj
    bpy.context.scene.render.engine = "CYCLES"
    bake = bpy.context.scene.render.bake
    bake.normal_space = "TANGENT"
    bake.normal_r = "POS_X"; bake.normal_g = "NEG_Y"; bake.normal_b = "POS_Z"
    bpy.ops.object.bake(type="NORMAL", normal_space="TANGENT", margin=16, use_clear=True)
    _save_png(target, target_path)
    bpy.data.materials.remove(material, do_unlink=True)
    bpy.data.images.remove(target, do_unlink=True)


def _bake_transfer(root: pathlib.Path, out: pathlib.Path, obj, source_uv, target_uv, size: int):
    raise RuntimeError("Cycles transfer is withdrawn: rendered QA found blank masks. Use transfer_fidelity_textures.py on source and UV-applied JSON instead.")


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=pathlib.Path, help="iteration directory containing six JSON source meshes")
    parser.add_argument("--out", required=True, type=pathlib.Path, help="new UV override directory")
    parser.add_argument("--angle", type=float, default=88.0, help="Smart Project angle limit in degrees")
    parser.add_argument("--margin-px", type=float, default=4.0, help="global packing gutter in 2048-atlas pixels")
    parser.add_argument("--unwrap-mode", choices=("smart", "tree"), default="smart",
                        help="smart is the non-overlap production default; tree is retained for comparison")
    parser.add_argument("--size", type=int, default=2048, help="Base atlas edge in pixels")
    parser.add_argument("--bake-transfer", action="store_true", help="also bake new shared texture maps; CPU-intensive")
    args = parser.parse_args(argv)
    root = args.input.resolve(); out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete(use_global=False)
    sources, solids = _load_solids(root)
    groups = _group_equivalent_solids(solids)
    obj, source_uv, target_uv, references = _make_master(groups)
    margin_fraction = args.margin_px / args.size
    target_uv, seam_count = _unwrap_and_pack(obj, target_uv, args.angle, margin_fraction, args.unwrap_mode)
    # As with the target layer, reacquire the source layer after Blender's UV
    # operator invalidates RNA handles.  This matters only for the optional
    # texture-transfer path, but keeps it from sampling an invalid layer.
    source_uv = obj.data.uv_layers.get("UV_Source")
    if source_uv is None or target_uv is None:
        raise RuntimeError("UV unwrap did not retain the named source/target layers")
    repair = _detach_and_repack_overlaps(obj, target_uv, margin_fraction)
    target_uv = obj.data.uv_layers.active
    islands = _island_count(obj.data, target_uv)
    atlas_metrics = _atlas_metrics(obj.data, target_uv, args.size)
    _assign_target_uv(groups, obj, target_uv, references)
    overrides = _write_overrides(out, sources, solids)
    report = {
        "author": AUTHOR,
        "creator": AUTHOR,
        "asset": ASSET,
        "status": "UV_OVERRIDE_READY_NOT_YET_APPLIED_TO_GLB",
        "input": str(root),
        "input_sources": [{"name": source["name"], "sha256": source["sha256"]} for source in sources],
        "weld_policy": "position + component-local COLOR_0 only",
        "uv_convention": {"source_json": JSON_UV_CONVENTION,
                          "blender_working": BLENDER_UV_CONVENTION,
                          "override_output": JSON_UV_CONVENTION,
                          "transforms": ["source JSON -> Blender: (u, 1-v)",
                                         "Blender -> override JSON: (u, 1-v)"]},
        "source_faces": sum(len(source["data"]["faces"]) for source in sources),
        "solids": len(solids),
        "unique_geometry_groups": len(groups),
        "representative_faces": len(obj.data.polygons),
        "representative_vertices": len(obj.data.vertices),
        "uv_islands": islands,
        "explicit_seams": seam_count,
        "global_pack_margin_px": args.margin_px,
        "global_pack_margin_fraction": margin_fraction,
        "atlas_metrics": atlas_metrics,
        "local_overlap_repair": repair,
        "target": "under 1000 islands",
        "unwrap_policy": (f"Smart Project {args.angle:g} degrees plus global pack; non-overlap priority"
                            if args.unwrap_mode == "smart" else
                            "experimental per-solid face-tree seams, angle-based unwrap, and global pack"),
        "shared_layout": "exactly compatible component copies receive the representative UV coordinates",
        "normal_policy": "UV_Source DirectX normal transformed through Blender shader Normal Map and baked in target tangent basis with negative Y output",
        "texture_transfer": "not run" if not args.bake_transfer else "completed; normal detail retangent transfer included",
        "overrides": overrides,
        "limitations": [
            "Override files must be consumed by the existing skinned GLB exporter before this atlas reaches Unreal.",
            "No automatic geometry-only normal fallback exists: a missing or failed source-normal transfer fails the bake rather than silently losing micro-normal detail.",
            "No Unreal import, material, LOD, or gameplay acceptance is established by this tool.",
        ],
    }
    if args.bake_transfer:
        report["textures"] = _bake_transfer(root, out, obj, source_uv, target_uv, args.size)
    report_path = out / "uv-production-report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    bpy.context.scene["author"] = AUTHOR; bpy.context.scene["creator"] = AUTHOR
    bpy.context.scene["status"] = report["status"]
    bpy.ops.wm.save_as_mainfile(filepath=str(out / "Riftstalker_uv_representatives.blend"))
    print(json.dumps({k: report[k] for k in ("solids", "unique_geometry_groups", "uv_islands", "status")}, indent=2))


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    main(args)
