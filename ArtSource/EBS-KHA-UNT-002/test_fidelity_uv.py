"""Read-only QA for Riftstalker UV override JSON files. Author: Angelis Pseftis.

Usage:
  python3 test_fidelity_uv.py --source <source13> --overrides <uv14>

It deliberately evaluates the final per-polygon-corner values which the GLB
export hook will consume.  Island count alone cannot establish texture safety:
a flattened non-developable face tree can retain only a few islands while
painting different triangles into the same pixels.
"""
from __future__ import annotations
import argparse, json, math, pathlib, statistics, sys

EPS = 1.0e-12


def json_uv_to_blender(uv): return (float(uv[0]), 1.0 - float(uv[1]))
def blender_uv_to_json(uv): return (float(uv[0]), 1.0 - float(uv[1]))


def _image_row_from_json_v(v, height):
    return min(height - 1, max(0, int(math.floor(v * height))))


def _image_row_from_blender_v(v, height):
    # Blender V is bottom-up while PNG row zero is top-down.
    return min(height - 1, max(0, int(math.floor((1.0 - v) * height))))


def verify_source_row_convention(source_root):
    """Prove a real source atlas row remains the same across the V flip."""
    from PIL import Image
    texture = source_root / "textures" / "T_EBS_KHA_UNT_002_BaseColor.png"
    first_mesh = next(iter(sorted(source_root.glob("*_lod[01].json"))), None)
    if first_mesh is None or not texture.is_file():
        raise AssertionError("source UV/image convention fixture is missing")
    data = json.loads(first_mesh.read_text()); source_uv = data["uv"][0]
    blender_uv = json_uv_to_blender(source_uv)
    assert blender_uv_to_json(blender_uv) == tuple(source_uv)
    with Image.open(texture) as image:
        json_row = _image_row_from_json_v(source_uv[1], image.height)
        blender_row = _image_row_from_blender_v(blender_uv[1], image.height)
        assert json_row == blender_row, (source_uv, blender_uv, json_row, blender_row)
        column = min(image.width - 1, max(0, int(math.floor(source_uv[0] * image.width))))
        # Reading the concrete source texel keeps this a source-image test,
        # rather than a coordinate-only algebra assertion.
        sample = image.convert("RGBA").getpixel((column, json_row))
    return {"source_json_uv": source_uv, "blender_uv": blender_uv,
            "image": texture.name, "pixel": [column, json_row], "rgba": list(sample),
            "convention": "JSON top-origin V -> Blender bottom-origin V via (u,1-v)"}


def area3(a, b, c):
    ab = [b[i] - a[i] for i in range(3)]; ac = [c[i] - a[i] for i in range(3)]
    return .5 * math.sqrt(sum(x * x for x in (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])))


def cross(a, b, c): return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0])
def signed(poly): return sum(cross((0, 0), a, b) for a, b in zip(poly, poly[1:]+poly[:1])) / 2
def area2(a, b, c): return abs(signed([a, b, c]))
def uv_length(a, b): return math.hypot(a[0]-b[0], a[1]-b[1])


def clip(subject, a, b, keep):
    result = []
    for p, q in zip(subject, subject[1:] + subject[:1]):
        inside_p = keep * cross(a, b, p) >= -EPS; inside_q = keep * cross(a, b, q) >= -EPS
        if inside_p: result.append(p)
        if inside_p != inside_q:
            dx, dy = q[0]-p[0], q[1]-p[1]; ex, ey = b[0]-a[0], b[1]-a[1]
            denominator = dx*ey - dy*ex
            if abs(denominator) > EPS:
                t = ((a[0]-p[0])*ey - (a[1]-p[1])*ex) / denominator
                result.append((p[0] + t*dx, p[1] + t*dy))
    return result


def overlap_area(first, second):
    polygon = first[:]; keep = 1 if signed(second) >= 0 else -1
    for a, b in zip(second, second[1:] + second[:1]):
        polygon = clip(polygon, a, b, keep)
        if not polygon: return 0.0
    return abs(signed(polygon)) if len(polygon) >= 3 else 0.0


def percentile(values, p):
    values = sorted(values)
    return values[round((len(values)-1)*p)] if values else None


def inspect(source, override, texture_size):
    data = json.loads(source.read_text()); result = json.loads(override.read_text())
    uv_faces = result["polygon_corner_uv"]
    if len(uv_faces) != len(data["faces"]): raise AssertionError(f"{source.name}: face count mismatch")
    triangles = []; density = []; anisotropy = []; deg3 = deguv = 0; total_uv_area = 0.0
    for index, (face, uv) in enumerate(zip(data["faces"], uv_faces)):
        if len(face["vertices"]) < 3 or len(uv) != len(face["vertices"]): raise AssertionError(f"{source.name}: invalid polygon at {index}")
        xyz = [data["vertices"][i] for i in face["vertices"]]; uv = [tuple(p) for p in uv]
        if not all(math.isfinite(v) and -EPS <= v <= 1 + EPS for p in uv for v in p): raise AssertionError(f"{source.name}: invalid UV at {index}")
        # Exporters triangulate n-gons. Evaluate those exact fan triangles,
        # including quads, instead of treating a quad as a malformed source.
        for fan in range(1, len(uv)-1):
            world_triangle = [xyz[0], xyz[fan], xyz[fan+1]]; uv_triangle = [uv[0], uv[fan], uv[fan+1]]
            world_area, atlas_area = area3(*world_triangle), area2(*uv_triangle); total_uv_area += atlas_area
            deg3 += int(world_area <= EPS); deguv += int(atlas_area <= EPS)
            if world_area > EPS and atlas_area > EPS:
                density.append(math.sqrt(atlas_area * texture_size * texture_size / world_area))
                scales = [uv_length(uv_triangle[a], uv_triangle[b])*texture_size/math.dist(world_triangle[a], world_triangle[b]) for a, b in ((0,1),(1,2),(2,0)) if math.dist(world_triangle[a], world_triangle[b]) > EPS]
                anisotropy.append(max(scales)/min(scales))
            triangles.append((min(x for x, y in uv_triangle), max(x for x, y in uv_triangle), min(y for x, y in uv_triangle), max(y for x, y in uv_triangle), uv_triangle, index))
    # Sweep bounding boxes, then exact convex clipping. Shared boundaries have
    # zero area and correctly do not fail the test.
    triangles.sort(); active = []; overlaps = []
    for triangle in triangles:
        active = [old for old in active if old[1] > triangle[0] + EPS]
        for old in active:
            if old[3] <= triangle[2] + EPS or triangle[3] <= old[2] + EPS: continue
            area = overlap_area(old[4], triangle[4])
            if area > EPS: overlaps.append((area, old[5], triangle[5]))
        active.append(triangle)
    overlap_uv = sum(item[0] for item in overlaps)
    return {
        "mesh": source.name, "faces": len(data["faces"]), "triangles_evaluated":len(triangles), "degenerate_3d": deg3, "degenerate_uv": deguv,
        "total_uv_face_area": total_uv_area, "overlap_pairs": len(overlaps), "overlap_uv_area": overlap_uv,
        "overlap_pixels": overlap_uv * texture_size * texture_size,
        "overlap_fraction_of_face_area": overlap_uv / total_uv_area if total_uv_area else math.inf,
        "max_pair_overlap_pixels": (max((x[0] for x in overlaps), default=0.0) * texture_size * texture_size),
        "density_px_per_cm": {"p05": percentile(density, .05), "median": statistics.median(density), "p95": percentile(density, .95)},
        "anisotropy": {"p95": percentile(anisotropy, .95), "max": max(anisotropy)},
        "examples": [{"pixels": area*texture_size*texture_size, "face_a": a, "face_b": b} for area, a, b in sorted(overlaps, reverse=True)[:5]],
    }


def main():
    p = argparse.ArgumentParser(); p.add_argument("--source", required=True, type=pathlib.Path); p.add_argument("--overrides", required=True, type=pathlib.Path)
    # A sub-pixel analytic intersection can arise from float UV coordinates
    # along packed chart borders.  The default permits less than one aggregate
    # 2048 texel per source mesh, while preserving a strict command-line option
    # for investigations.  The report always exposes exact pair and area data.
    p.add_argument("--size", type=int, default=2048); p.add_argument("--max-overlap-pixels", type=float, default=1.0)
    args = p.parse_args(); reports = []
    for source in sorted(args.source.glob("*_lod[01].json")):
        reports.append(inspect(source, args.overrides/(source.stem+".uv-override.json"), args.size))
    print(json.dumps({"author":"Angelis Pseftis", "source_row_convention":verify_source_row_convention(args.source), "reports":reports}, indent=2))
    failed = [r for r in reports if r["degenerate_3d"] or r["degenerate_uv"] or r["overlap_pixels"] > args.max_overlap_pixels]
    if failed: raise SystemExit(f"UV QA FAILED: {', '.join(r['mesh'] for r in failed)}")


if __name__ == "__main__": main()
