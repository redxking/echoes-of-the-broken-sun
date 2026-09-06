#!/usr/bin/env python3
"""ebs_render.py - review-view renderer for OBJ blockouts.

Renders orthographic (front/side/rear/left/top) and Unreal-style perspective
tactical views of triangle meshes stored in Wavefront OBJ files and writes
8-bit RGB PNGs plus a JSON manifest. Used to validate 3D blockouts against the
RTS camera before Unreal import.

World convention (Unreal, left-handed): +X forward, +Y right, +Z up, units in
centimeters. OBJ vertices are already in this frame; the OBJ file is a plain
container and no Y-up conversion is applied.

Standard library only. Deterministic: identical inputs give byte-identical PNGs.

Author: Angelis Pseftis
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
import sys
import time
import zlib

TOOL_NAME = "ebs_render.py"
AUTHOR = "Angelis Pseftis"
NEAR_PLANE_CM = 1.0
FEATURE_EDGE_DEG = 30.0
DEFAULT_MATERIAL = "_default"
DEFAULT_COLOR = (0.6, 0.6, 0.6)

AXIS_DIRS = {
    "+X": (1.0, 0.0, 0.0), "-X": (-1.0, 0.0, 0.0),
    "+Y": (0.0, 1.0, 0.0), "-Y": (0.0, -1.0, 0.0),
    "+Z": (0.0, 0.0, 1.0), "-Z": (0.0, 0.0, -1.0),
}


class RenderError(Exception):
    """Raised on bad input; reported as a clear message with non-zero exit."""


# --- vector helpers ----------------------------------------------------------
def v_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def v_add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def v_scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def v_dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def v_cross(a, b):
    """Standard component formula: a x b."""
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def v_length(a):
    return math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def v_normalize(a):
    length = v_length(a)
    if length == 0.0:
        return (0.0, 0.0, 0.0)
    return (a[0] / length, a[1] / length, a[2] / length)


# --- OBJ loading -------------------------------------------------------------
def load_obj(path):
    """Load an OBJ as (vertices, faces).

    vertices: list of (x, y, z) floats in the Unreal frame.
    faces: list of (i, j, k, material_name) triangles (polygons fan-triangulated).
    Supports v / vn / vt / f (i, i/t, i//n, i/t/n, negative indices), o / g,
    usemtl and comments. Other records are ignored.
    """
    verts = []
    faces = []
    material = DEFAULT_MATERIAL
    try:
        handle = open(path, "r", encoding="utf-8", errors="replace")
    except OSError as exc:
        raise RenderError(f"cannot open OBJ '{path}': {exc}") from exc
    with handle:
        for lineno, raw in enumerate(handle, 1):
            line = raw.strip()
            if not line or line[0] == "#":
                continue
            parts = line.split()
            tag = parts[0]
            if tag == "v":
                if len(parts) < 4:
                    raise RenderError(f"{path}:{lineno}: vertex needs 3 coordinates")
                try:
                    verts.append((float(parts[1]), float(parts[2]), float(parts[3])))
                except ValueError as exc:
                    raise RenderError(f"{path}:{lineno}: bad vertex '{line}'") from exc
            elif tag == "f":
                idx = []
                for tok in parts[1:]:
                    head = tok.split("/")[0]
                    if not head:
                        raise RenderError(f"{path}:{lineno}: bad face token '{tok}'")
                    try:
                        i = int(head)
                    except ValueError as exc:
                        raise RenderError(f"{path}:{lineno}: bad face token '{tok}'") from exc
                    if i < 0:
                        i = len(verts) + i
                    elif i > 0:
                        i -= 1
                    else:
                        raise RenderError(f"{path}:{lineno}: OBJ indices are 1-based (got 0)")
                    if i < 0 or i >= len(verts):
                        raise RenderError(f"{path}:{lineno}: face index {tok} out of range")
                    idx.append(i)
                if len(idx) < 3:
                    raise RenderError(f"{path}:{lineno}: face needs at least 3 indices")
                for k in range(1, len(idx) - 1):
                    faces.append((idx[0], idx[k], idx[k + 1], material))
            elif tag == "usemtl":
                material = parts[1] if len(parts) > 1 else DEFAULT_MATERIAL
            # vn, vt, vp, o, g, s, mtllib, l: ignored on purpose
    return verts, faces


# --- geometry batches ----------------------------------------------------------
class Batch:
    """A group of world-space triangles sharing one edge/lighting policy.

    tris: list of (i, j, k, (r, g, b) base color 0..1, unlit flag)
    """

    __slots__ = ("name", "verts", "tris", "draw_edges", "_edges", "_normals")

    def __init__(self, name, verts, tris, draw_edges):
        self.name = name
        self.verts = verts
        self.tris = tris
        self.draw_edges = draw_edges
        self._edges = None
        self._normals = None

    def normals(self):
        if self._normals is None:
            out = []
            verts = self.verts
            for i, j, k, _c, _u in self.tris:
                a, b, c = verts[i], verts[j], verts[k]
                out.append(v_normalize(v_cross(v_sub(b, a), v_sub(c, a))))
            self._normals = out
        return self._normals

    def feature_edges(self, angle_deg=FEATURE_EDGE_DEG):
        """Boundary edges plus edges whose adjacent face normals differ > angle."""
        if self._edges is not None:
            return self._edges
        weld = {}
        weld_id = []
        for v in self.verts:
            key = (round(v[0], 3), round(v[1], 3), round(v[2], 3))
            weld_id.append(weld.setdefault(key, len(weld)))
        normals = self.normals()
        edge_map = {}
        for t, (i, j, k, _c, _u) in enumerate(self.tris):
            for a, b in ((i, j), (j, k), (k, i)):
                wa, wb = weld_id[a], weld_id[b]
                if wa == wb:
                    continue
                key = (wa, wb) if wa < wb else (wb, wa)
                entry = edge_map.get(key)
                if entry is None:
                    edge_map[key] = [(a, b), [t]]
                else:
                    entry[1].append(t)
        cos_limit = math.cos(math.radians(angle_deg))
        edges = []
        for (a, b), face_ids in edge_map.values():
            if len(face_ids) != 2:
                edges.append((a, b))
                continue
            n0 = normals[face_ids[0]]
            n1 = normals[face_ids[1]]
            if abs(v_dot(n0, n1)) < cos_limit:
                edges.append((a, b))
        self._edges = edges
        return edges


def _color3(value, what):
    if (not isinstance(value, (list, tuple))) or len(value) != 3:
        raise RenderError(f"{what}: expected [r, g, b]")
    try:
        return (float(value[0]), float(value[1]), float(value[2]))
    except (TypeError, ValueError) as exc:
        raise RenderError(f"{what}: expected numeric [r, g, b]") from exc


def _vec3(value, what):
    return _color3(value, what)


def transform_vertices(verts, translate, yaw_deg, scale):
    yaw = math.radians(yaw_deg)
    c, s = math.cos(yaw), math.sin(yaw)
    tx, ty, tz = translate
    out = []
    for x, y, z in verts:
        x *= scale
        y *= scale
        z *= scale
        # positive yaw turns +X toward +Y
        rx = x * c - y * s
        ry = x * s + y * c
        out.append((rx + tx, ry + ty, z + tz))
    return out


def build_mesh_batch(name, verts, faces, materials, emissive):
    tris = []
    default = materials.get(DEFAULT_MATERIAL, DEFAULT_COLOR)
    for i, j, k, mat in faces:
        color = materials.get(mat, default)
        tris.append((i, j, k, color, mat in emissive))
    return Batch(name, verts, tris, True)


def box_batch(name, center, size, color, unlit=False, draw_edges=True):
    cx, cy, cz = center
    hx, hy, hz = size[0] * 0.5, size[1] * 0.5, size[2] * 0.5
    verts = [
        (cx - hx, cy - hy, cz - hz), (cx + hx, cy - hy, cz - hz),
        (cx + hx, cy + hy, cz - hz), (cx - hx, cy + hy, cz - hz),
        (cx - hx, cy - hy, cz + hz), (cx + hx, cy - hy, cz + hz),
        (cx + hx, cy + hy, cz + hz), (cx - hx, cy + hy, cz + hz),
    ]
    quads = [
        (0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
        (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7),
    ]
    tris = []
    for a, b, c, d in quads:
        tris.append((a, b, c, color, unlit))
        tris.append((a, c, d, color, unlit))
    return Batch(name, verts, tris, draw_edges)


def _add_quad(verts, tris, p0, p1, p2, p3, color, unlit):
    base = len(verts)
    verts.extend((p0, p1, p2, p3))
    tris.append((base, base + 1, base + 2, color, unlit))
    tris.append((base, base + 2, base + 3, color, unlit))


def _add_strip(verts, tris, x0, y0, x1, y1, z, width, color, seg_cm):
    """Thin horizontal quad from (x0,y0) to (x1,y1) at height z, split into segments."""
    dx, dy = x1 - x0, y1 - y0
    length = math.hypot(dx, dy)
    if length == 0.0:
        return
    ux, uy = dx / length, dy / length
    nx, ny = -uy * width * 0.5, ux * width * 0.5
    segments = max(1, int(math.ceil(length / seg_cm)))
    for s in range(segments):
        t0 = s / segments
        t1 = (s + 1) / segments
        ax, ay = x0 + dx * t0, y0 + dy * t0
        bx, by = x0 + dx * t1, y0 + dy * t1
        _add_quad(
            verts, tris,
            (ax - nx, ay - ny, z), (bx - nx, by - ny, z),
            (bx + nx, by + ny, z), (ax + nx, ay + ny, z),
            color, True,
        )


def build_ground_batches(ground, line_width_cm):
    """Subdivided ground plane, grid line strips and footprint outline."""
    if not ground:
        return []
    tile = float(ground.get("tile_cm", 200.0))
    tiles = int(ground.get("tiles", 12))
    if tile <= 0 or tiles <= 0:
        raise RenderError("ground.tile_cm and ground.tiles must be positive")
    color = _color3(ground.get("color", [0.22, 0.23, 0.25]), "ground.color")
    grid_color = _color3(ground.get("grid_color", [0.34, 0.36, 0.40]), "ground.grid_color")
    half = tile * tiles * 0.5
    verts = []
    tris = []
    for iy in range(tiles):
        y0 = -half + iy * tile
        y1 = y0 + tile
        for ix in range(tiles):
            x0 = -half + ix * tile
            x1 = x0 + tile
            _add_quad(verts, tris, (x0, y0, 0.0), (x1, y0, 0.0), (x1, y1, 0.0), (x0, y1, 0.0), color, False)
    batches = [Batch("ground", verts, tris, False)]

    gverts = []
    gtris = []
    for i in range(tiles + 1):
        c = -half + i * tile
        _add_strip(gverts, gtris, c, -half, c, half, 2.0, line_width_cm, grid_color, tile)
        _add_strip(gverts, gtris, -half, c, half, c, 2.0, line_width_cm, grid_color, tile)
    batches.append(Batch("ground_grid", gverts, gtris, False))

    footprint = ground.get("footprint_cm")
    if footprint:
        if (not isinstance(footprint, (list, tuple))) or len(footprint) != 2:
            raise RenderError("ground.footprint_cm: expected [x_cm, y_cm]")
        fx, fy = float(footprint[0]) * 0.5, float(footprint[1]) * 0.5
        fcolor = _color3(ground.get("footprint_color", [0.95, 0.62, 0.18]), "ground.footprint_color")
        fwidth = float(ground.get("footprint_line_cm", 6.0))
        fverts = []
        ftris = []
        corners = [(-fx, -fy), (fx, -fy), (fx, fy), (-fx, fy)]
        for n in range(4):
            ax, ay = corners[n]
            bx, by = corners[(n + 1) % 4]
            _add_strip(fverts, ftris, ax, ay, bx, by, 3.0, fwidth, fcolor, tile)
        batches.append(Batch("ground_footprint", fverts, ftris, False))
    return batches


def build_reference_figure(figure):
    if not figure:
        return None
    height = float(figure.get("height_cm", 180.0))
    pos = _vec3(figure.get("position", [0.0, 0.0, 0.0]), "reference_figure.position")
    color = _color3(figure.get("color", [0.92, 0.56, 0.20]), "reference_figure.color")
    center = (pos[0], pos[1], pos[2] + height * 0.5)
    return box_batch("reference_figure", center, (40.0, 40.0, height), color, False, True)


def union_bbox(batches):
    lo = [math.inf, math.inf, math.inf]
    hi = [-math.inf, -math.inf, -math.inf]
    found = False
    for batch in batches:
        for x, y, z in batch.verts:
            found = True
            if x < lo[0]:
                lo[0] = x
            if y < lo[1]:
                lo[1] = y
            if z < lo[2]:
                lo[2] = z
            if x > hi[0]:
                hi[0] = x
            if y > hi[1]:
                hi[1] = y
            if z > hi[2]:
                hi[2] = z
    if not found:
        return None
    return (tuple(lo), tuple(hi))


# --- cameras -------------------------------------------------------------------
def ortho_axes(from_side, image_up="+X"):
    """Camera basis for an ortho view: camera sits on `from_side`, looks at origin."""
    if from_side not in AXIS_DIRS:
        raise RenderError(f"ortho 'from' must be one of {sorted(AXIS_DIRS)} (got {from_side!r})")
    side = AXIS_DIRS[from_side]
    forward = (-side[0], -side[1], -side[2])
    if from_side in ("+Z", "-Z"):
        if image_up not in AXIS_DIRS:
            raise RenderError(f"ortho 'image_up' must be one of {sorted(AXIS_DIRS)} (got {image_up!r})")
        up_hint = AXIS_DIRS[image_up]
    else:
        up_hint = (0.0, 0.0, 1.0)
    right = v_normalize(v_cross(up_hint, forward))
    if v_length(right) == 0.0:
        raise RenderError(f"ortho view from {from_side}: up hint {image_up} is parallel to the view")
    up = v_cross(forward, right)
    return forward, right, up


def perspective_camera(pitch_deg, yaw_deg, arm_cm, target):
    """Unreal FRotator(pitch, yaw, roll=0) spring-arm camera.

    Returns (position, forward, right, up).
    """
    p = math.radians(pitch_deg)
    y = math.radians(yaw_deg)
    forward = (math.cos(p) * math.cos(y), math.cos(p) * math.sin(y), math.sin(p))
    right = (-math.sin(y), math.cos(y), 0.0)
    up = v_cross(forward, right)
    position = v_sub(target, v_scale(forward, arm_cm))
    return position, forward, right, up


# --- rasterization ---------------------------------------------------------------
def _raster_triangle(x0, y0, d0, x1, y1, d1, x2, y2, d2, color, zb, cb, W, H):
    """Flat-colored, z-tested triangle. Screen coords are float pixels; depth is
    linear in screen space (zc for ortho, -1/zc for perspective)."""
    area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
    if area == 0.0:
        return
    if area < 0.0:
        x1, y1, d1, x2, y2, d2 = x2, y2, d2, x1, y1, d1
        area = -area
    minx = int(math.floor(min(x0, x1, x2)))
    maxx = int(math.ceil(max(x0, x1, x2)))
    miny = int(math.floor(min(y0, y1, y2)))
    maxy = int(math.ceil(max(y0, y1, y2)))
    if minx < 0:
        minx = 0
    if miny < 0:
        miny = 0
    if maxx > W - 1:
        maxx = W - 1
    if maxy > H - 1:
        maxy = H - 1
    if minx > maxx or miny > maxy:
        return
    # e(p) = A*px + B*py + C ; inside when all three >= 0 (area > 0 orientation)
    A0 = y1 - y2
    B0 = x2 - x1
    C0 = -B0 * y1 - A0 * x1
    A1 = y2 - y0
    B1 = x0 - x2
    C1 = -B1 * y2 - A1 * x2
    A2 = y0 - y1
    B2 = x1 - x0
    C2 = -B2 * y0 - A2 * x0
    inv = 1.0 / area
    Dx = (A0 * d0 + A1 * d1 + A2 * d2) * inv
    Dy = (B0 * d0 + B1 * d1 + B2 * d2) * inv
    Dc = (C0 * d0 + C1 * d1 + C2 * d2) * inv
    ceil = math.ceil
    floor = math.floor
    hA0, hA1, hA2 = A0 * 0.5, A1 * 0.5, A2 * 0.5
    for py in range(miny, maxy + 1):
        yc = py + 0.5
        lo = minx
        hi = maxx
        e = B0 * yc + C0 + hA0
        if A0 > 0.0:
            t = ceil(-e / A0)
            if t > lo:
                lo = t
        elif A0 < 0.0:
            t = floor(-e / A0)
            if t < hi:
                hi = t
        elif e < 0.0:
            continue
        e = B1 * yc + C1 + hA1
        if A1 > 0.0:
            t = ceil(-e / A1)
            if t > lo:
                lo = t
        elif A1 < 0.0:
            t = floor(-e / A1)
            if t < hi:
                hi = t
        elif e < 0.0:
            continue
        e = B2 * yc + C2 + hA2
        if A2 > 0.0:
            t = ceil(-e / A2)
            if t > lo:
                lo = t
        elif A2 < 0.0:
            t = floor(-e / A2)
            if t < hi:
                hi = t
        elif e < 0.0:
            continue
        if lo > hi:
            continue
        d = Dx * (lo + 0.5) + Dy * yc + Dc
        base = py * W
        for idx in range(base + lo, base + hi + 1):
            if d < zb[idx]:
                zb[idx] = d
                cb[idx] = color
            d += Dx


def _clip_segment(x0, y0, x1, y1, W, H):
    """Liang-Barsky clip against [0, W) x [0, H). Returns (t0, t1) or None."""
    dx = x1 - x0
    dy = y1 - y0
    t0, t1 = 0.0, 1.0
    for p, q in ((-dx, x0), (dx, W - 1e-6 - x0), (-dy, y0), (dy, H - 1e-6 - y0)):
        if p == 0.0:
            if q < 0.0:
                return None
            continue
        r = q / p
        if p < 0.0:
            if r > t1:
                return None
            if r > t0:
                t0 = r
        else:
            if r < t0:
                return None
            if r < t1:
                t1 = r
    return t0, t1


def _draw_line(x0, y0, d0, x1, y1, d1, color, zb, cb, W, H, bias_c, bias_q):
    """Integer Bresenham with per-pixel depth, z-tested with bias (no z write)."""
    clip = _clip_segment(x0, y0, x1, y1, W, H)
    if clip is None:
        return
    t0, t1 = clip
    if t1 < t0:
        return
    dx = x1 - x0
    dy = y1 - y0
    dd = d1 - d0
    ax, ay, ad = x0 + dx * t0, y0 + dy * t0, d0 + dd * t0
    bx, by, bd = x0 + dx * t1, y0 + dy * t1, d0 + dd * t1
    ix, iy = int(ax), int(ay)
    jx, jy = int(bx), int(by)
    if ix < 0:
        ix = 0
    if iy < 0:
        iy = 0
    if jx < 0:
        jx = 0
    if jy < 0:
        jy = 0
    if ix > W - 1:
        ix = W - 1
    if jx > W - 1:
        jx = W - 1
    if iy > H - 1:
        iy = H - 1
    if jy > H - 1:
        jy = H - 1
    adx = abs(jx - ix)
    ady = -abs(jy - iy)
    sx = 1 if ix < jx else -1
    sy = 1 if iy < jy else -1
    err = adx + ady
    steps = max(adx, -ady)
    step_d = (bd - ad) / steps if steps > 0 else 0.0
    d = ad
    x, y = ix, iy
    while True:
        idx = y * W + x
        if d <= zb[idx] + bias_c + bias_q * d * d:
            cb[idx] = color
        if x == jx and y == jy:
            break
        e2 = 2 * err
        if e2 >= ady:
            err += ady
            x += sx
        if e2 <= adx:
            err += adx
            y += sy
        d += step_d


def _pack(rgb):
    r = int(rgb[0] * 255.0 + 0.5)
    g = int(rgb[1] * 255.0 + 0.5)
    b = int(rgb[2] * 255.0 + 0.5)
    r = 0 if r < 0 else (255 if r > 255 else r)
    g = 0 if g < 0 else (255 if g > 255 else g)
    b = 0 if b < 0 else (255 if b > 255 else b)
    return (r << 16) | (g << 8) | b


# --- lighting --------------------------------------------------------------------
class Lighting:
    __slots__ = ("direction", "ambient", "key", "color", "fill_color", "fill", "_amb")

    def __init__(self, cfg):
        cfg = cfg or {}
        self.direction = v_normalize(_vec3(cfg.get("direction", [-0.45, 0.35, -0.82]), "light.direction"))
        if v_length(self.direction) == 0.0:
            raise RenderError("light.direction must be non-zero")
        self.ambient = float(cfg.get("ambient", 0.35))
        self.key = float(cfg.get("key", 0.75))
        self.color = _color3(cfg.get("color", [1.0, 0.82, 0.62]), "light.color")
        self.fill_color = _color3(cfg.get("fill_color", [0.48, 0.60, 0.88]), "light.fill_color")
        self.fill = float(cfg.get("fill", 0.25))
        fc = self.fill_color
        # ambient tinted halfway toward the fill color
        self._amb = tuple(self.ambient * (0.5 + 0.5 * fc[i]) for i in range(3))

    def shade(self, base, normal):
        """Flat shading. `normal` must already face the camera."""
        L = self.direction
        ndl = -(normal[0] * L[0] + normal[1] * L[1] + normal[2] * L[2])
        key = self.key * ndl if ndl > 0.0 else 0.0
        fill = self.fill * -ndl if ndl < 0.0 else 0.0
        amb = self._amb
        kc = self.color
        fc = self.fill_color
        out = []
        for i in range(3):
            v = base[i] * (amb[i] + key * kc[i] + fill * fc[i])
            out.append(v if v < 1.0 else 1.0)
        return out


# --- view rendering -------------------------------------------------------------------
class ViewSetup:
    __slots__ = ("kind", "position", "forward", "right", "up", "width_cm", "height_cm",
                 "tan_half", "aspect", "bias_c", "bias_q", "line_width_cm", "target")


def setup_view(view, bbox, W, H):
    kind = view.get("type")
    aspect = W / H
    vs = ViewSetup()
    vs.kind = kind
    vs.aspect = aspect
    if kind == "ortho":
        forward, right, up = ortho_axes(view.get("from", "+X"), view.get("image_up", "+X"))
        target = _vec3(view.get("target", [0.0, 0.0, 0.0]), "view.target")
        margin = float(view.get("margin", 1.15))
        if "ortho_width_cm" in view:
            width_cm = float(view["ortho_width_cm"])
            if width_cm <= 0:
                raise RenderError(f"view {view.get('name')}: ortho_width_cm must be positive")
        else:
            if bbox is None:
                width_cm = 1000.0
            else:
                lo, hi = bbox
                hx = hy = 0.0
                for cx in (lo[0], hi[0]):
                    for cy in (lo[1], hi[1]):
                        for cz in (lo[2], hi[2]):
                            d = (cx - target[0], cy - target[1], cz - target[2])
                            hx = max(hx, abs(v_dot(d, right)))
                            hy = max(hy, abs(v_dot(d, up)))
                width_cm = max(2.0 * hx * margin, 2.0 * hy * margin * aspect, 1.0)
        diag = 1000.0
        if bbox is not None:
            diag = max(diag, v_length(v_sub(bbox[1], bbox[0])) * 2.0 + 1000.0)
        vs.position = v_sub(target, v_scale(forward, diag))
        vs.width_cm = width_cm
        vs.height_cm = width_cm / aspect
        vs.tan_half = 0.0
        vs.bias_c = 0.0025 * width_cm
        vs.bias_q = 0.0
        vs.line_width_cm = max(2.0, 1.5 * width_cm / W)
    elif kind == "persp":
        target = _vec3(view.get("target", [0.0, 0.0, 0.0]), "view.target")
        pitch = float(view.get("pitch_deg", -48.0))
        yaw = float(view.get("yaw_deg", -45.0))
        arm = float(view.get("arm_cm", 3800.0))
        fov = float(view.get("fov_deg", 55.0))
        if arm <= 0 or not (0.0 < fov < 180.0):
            raise RenderError(f"view {view.get('name')}: arm_cm must be > 0 and 0 < fov_deg < 180")
        position, forward, right, up = perspective_camera(pitch, yaw, arm, target)
        vs.position = position
        vs.tan_half = math.tan(math.radians(fov) * 0.5)
        vs.width_cm = 2.0 * arm * vs.tan_half
        vs.height_cm = vs.width_cm / aspect
        vs.bias_c = 0.0
        vs.bias_q = 0.0025 * arm
        vs.line_width_cm = max(2.0, 1.5 * vs.width_cm / W)
    else:
        raise RenderError(f"view {view.get('name')!r}: type must be 'ortho' or 'persp' (got {kind!r})")
    vs.forward = forward
    vs.right = right
    vs.up = up
    vs.target = target
    return vs


def _project(vs, verts, W, H):
    """Project world vertices. Returns (px, py, depth, valid) parallel lists."""
    cx, cy, cz = vs.position
    fx, fy, fz = vs.forward
    rx, ry, rz = vs.right
    ux, uy, uz = vs.up
    n = len(verts)
    PX = [0.0] * n
    PY = [0.0] * n
    PD = [0.0] * n
    OK = [True] * n
    halfW = 0.5 * W
    halfH = 0.5 * H
    if vs.kind == "ortho":
        sx = halfW / (vs.width_cm * 0.5)
        sy = halfH / (vs.height_cm * 0.5)
        for i in range(n):
            x, y, z = verts[i]
            dx, dy, dz = x - cx, y - cy, z - cz
            xc = dx * rx + dy * ry + dz * rz
            yc = dx * ux + dy * uy + dz * uz
            zc = dx * fx + dy * fy + dz * fz
            PX[i] = halfW + xc * sx
            PY[i] = halfH - yc * sy
            PD[i] = zc
    else:
        inv_t = 1.0 / vs.tan_half
        inv_ty = vs.aspect / vs.tan_half
        near = NEAR_PLANE_CM
        for i in range(n):
            x, y, z = verts[i]
            dx, dy, dz = x - cx, y - cy, z - cz
            zc = dx * fx + dy * fy + dz * fz
            if zc <= near:
                OK[i] = False
                continue
            xc = dx * rx + dy * ry + dz * rz
            yc = dx * ux + dy * uy + dz * uz
            PX[i] = halfW + (xc / zc) * inv_t * halfW
            PY[i] = halfH - (yc / zc) * inv_ty * halfH
            PD[i] = -1.0 / zc
    return PX, PY, PD, OK


def render_view(view, batches, bbox, lighting, background, W, H, ground_cfg):
    """Render one view; returns (packed color list, ViewSetup)."""
    vs = setup_view(view, bbox, W, H)
    all_batches = list(batches) + build_ground_batches(ground_cfg, vs.line_width_cm)
    bg = _pack(background)
    zb = [math.inf] * (W * H)
    cb = [bg] * (W * H)
    raster = _raster_triangle
    persp = vs.kind == "persp"
    cam = vs.position
    fwd = vs.forward
    projected = []
    for batch in all_batches:
        PX, PY, PD, OK = _project(vs, batch.verts, W, H)
        projected.append((PX, PY, PD, OK))
        normals = batch.normals()
        verts = batch.verts
        color_cache = {}
        for t, (i, j, k, base, unlit) in enumerate(batch.tris):
            if persp and not (OK[i] and OK[j] and OK[k]):
                continue
            n = normals[t]
            if unlit:
                key = (base, None)
                color = color_cache.get(key)
                if color is None:
                    color = _pack(base)
                    color_cache[key] = color
            else:
                if persp:
                    a = verts[i]
                    b = verts[j]
                    c = verts[k]
                    vx = cam[0] - (a[0] + b[0] + c[0]) / 3.0
                    vy = cam[1] - (a[1] + b[1] + c[1]) / 3.0
                    vz = cam[2] - (a[2] + b[2] + c[2]) / 3.0
                    facing = n[0] * vx + n[1] * vy + n[2] * vz
                else:
                    facing = -(n[0] * fwd[0] + n[1] * fwd[1] + n[2] * fwd[2])
                if facing < 0.0:
                    n = (-n[0], -n[1], -n[2])
                key = (base, (round(n[0], 4), round(n[1], 4), round(n[2], 4)))
                color = color_cache.get(key)
                if color is None:
                    color = _pack(lighting.shade(base, n))
                    color_cache[key] = color
            raster(PX[i], PY[i], PD[i], PX[j], PY[j], PD[j], PX[k], PY[k], PD[k], color, zb, cb, W, H)
    if view.get("edges"):
        edge_color = _pack(_color3(view.get("edge_color", [0.05, 0.05, 0.07]), "view.edge_color"))
        for batch, (PX, PY, PD, OK) in zip(all_batches, projected):
            if not batch.draw_edges:
                continue
            for a, b in batch.feature_edges():
                if persp and not (OK[a] and OK[b]):
                    continue
                _draw_line(PX[a], PY[a], PD[a], PX[b], PY[b], PD[b], edge_color,
                           zb, cb, W, H, vs.bias_c, vs.bias_q)
    if view.get("grayscale"):
        cache = {}
        for idx in range(len(cb)):
            c = cb[idx]
            g = cache.get(c)
            if g is None:
                r = c >> 16
                gg = (c >> 8) & 255
                bb = c & 255
                y = int(0.2126 * r + 0.7152 * gg + 0.0722 * bb + 0.5)
                y = 255 if y > 255 else y
                g = (y << 16) | (y << 8) | y
                cache[c] = g
            cb[idx] = g
    return cb, vs


# --- PNG ---------------------------------------------------------------------------------
def encode_png(packed, W, H):
    """8-bit RGB PNG, filter 0 on every row, zlib level 6, no ancillary chunks."""
    raw = bytearray()
    fmt = ">%dI" % W
    for y in range(H):
        row = bytearray(struct.pack(fmt, *packed[y * W:(y + 1) * W]))
        del row[0::4]
        raw.append(0)
        raw += row
    compressed = zlib.compress(bytes(raw), 6)

    def chunk(tag, payload):
        body = tag + payload
        return struct.pack(">I", len(payload)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", compressed) + chunk(b"IEND", b"")


def decode_png(data):
    """Minimal decoder for verification: returns (W, H, list of packed ints).
    Only accepts the layout written by encode_png (8-bit RGB, filter 0)."""
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise RenderError("not a PNG")
    pos = 8
    W = H = None
    idat = bytearray()
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        crc = struct.unpack(">I", data[pos + 8 + length:pos + 12 + length])[0]
        if zlib.crc32(tag + payload) & 0xFFFFFFFF != crc:
            raise RenderError(f"PNG chunk {tag!r}: CRC mismatch")
        if tag == b"IHDR":
            W, H, depth, ctype = struct.unpack(">IIBB", payload[:10])
            if depth != 8 or ctype != 2:
                raise RenderError("decode_png only supports 8-bit RGB")
        elif tag == b"IDAT":
            idat += payload
        elif tag == b"IEND":
            break
        pos += 12 + length
    if W is None:
        raise RenderError("PNG has no IHDR")
    raw = zlib.decompress(bytes(idat))
    stride = 1 + 3 * W
    if len(raw) != stride * H:
        raise RenderError("PNG payload size mismatch")
    pixels = []
    for y in range(H):
        if raw[y * stride] != 0:
            raise RenderError("decode_png only supports filter 0")
        row = raw[y * stride + 1:(y + 1) * stride]
        for x in range(W):
            pixels.append((row[3 * x] << 16) | (row[3 * x + 1] << 8) | row[3 * x + 2])
    return W, H, pixels


# --- scene -------------------------------------------------------------------------------
def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for block in iter(lambda: fh.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def load_scene(scene_path):
    try:
        with open(scene_path, "r", encoding="utf-8") as fh:
            scene = json.load(fh)
    except OSError as exc:
        raise RenderError(f"cannot open scene '{scene_path}': {exc}") from exc
    except ValueError as exc:
        raise RenderError(f"scene '{scene_path}' is not valid JSON: {exc}") from exc
    if not isinstance(scene, dict):
        raise RenderError("scene JSON must be an object")
    meshes = scene.get("meshes", [])
    if not isinstance(meshes, list):
        raise RenderError("scene.meshes must be a list")
    views = scene.get("views")
    if not isinstance(views, list) or not views:
        raise RenderError("scene.views must be a non-empty list")
    seen = set()
    for view in views:
        if not isinstance(view, dict) or not isinstance(view.get("name"), str) or not view["name"]:
            raise RenderError("every view needs a string 'name'")
        name = view["name"]
        if any(ch not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-." for ch in name):
            raise RenderError(f"view name {name!r}: use only letters, digits, '_', '-', '.'")
        if name in seen:
            raise RenderError(f"duplicate view name {name!r}")
        seen.add(name)
        if view.get("type") not in ("ortho", "persp"):
            raise RenderError(f"view {name!r}: type must be 'ortho' or 'persp'")
    width = int(scene.get("width", 1920))
    height = int(scene.get("height", 1080))
    if width <= 0 or height <= 0:
        raise RenderError("scene.width and scene.height must be positive")
    return scene


def build_scene_batches(scene, scene_dir):
    materials_cfg = scene.get("materials", {}) or {}
    if not isinstance(materials_cfg, dict):
        raise RenderError("scene.materials must be an object")
    materials = {name: _color3(rgb, f"materials.{name}") for name, rgb in materials_cfg.items()}
    emissive = set(scene.get("emissive", []) or [])
    batches = []
    mesh_records = []
    for index, mesh in enumerate(scene.get("meshes", [])):
        if not isinstance(mesh, dict) or "obj" not in mesh:
            raise RenderError(f"meshes[{index}]: needs an 'obj' path")
        obj_path = mesh["obj"]
        if not os.path.isabs(obj_path):
            obj_path = os.path.normpath(os.path.join(scene_dir, obj_path))
        verts, faces = load_obj(obj_path)
        if not faces:
            raise RenderError(f"meshes[{index}]: '{obj_path}' has no faces")
        translate = _vec3(mesh.get("translate", [0.0, 0.0, 0.0]), f"meshes[{index}].translate")
        yaw = float(mesh.get("yaw_deg", 0.0))
        scale = float(mesh.get("scale", 1.0))
        world = transform_vertices(verts, translate, yaw, scale)
        batches.append(build_mesh_batch(f"mesh{index}", world, faces, materials, emissive))
        mesh_records.append({
            "obj": obj_path,
            "sha256": sha256_file(obj_path),
            "triangles": len(faces),
            "translate": list(translate),
            "yaw_deg": yaw,
        })
    return batches, mesh_records


def render_scene(scene_path, out_dir, scale=1.0, view_names=None, log=None):
    """Render every requested view; returns the manifest dict."""
    if scale <= 0:
        raise RenderError("--scale must be positive")
    scene = load_scene(scene_path)
    scene_dir = os.path.dirname(os.path.abspath(scene_path))
    mesh_batches, mesh_records = build_scene_batches(scene, scene_dir)
    bbox = union_bbox(mesh_batches)
    figure = build_reference_figure(scene.get("reference_figure"))
    batches = list(mesh_batches)
    if figure is not None:
        batches.append(figure)
    lighting = Lighting(scene.get("light"))
    background = _color3(scene.get("background", [0.11, 0.12, 0.14]), "background")
    ground_cfg = scene.get("ground")
    W = max(1, int(round(int(scene.get("width", 1920)) * scale)))
    H = max(1, int(round(int(scene.get("height", 1080)) * scale)))

    views = scene["views"]
    if view_names:
        by_name = {v["name"]: v for v in views}
        missing = [n for n in view_names if n not in by_name]
        if missing:
            raise RenderError(f"unknown view(s): {', '.join(missing)}; available: {', '.join(by_name)}")
        views = [by_name[n] for n in view_names]

    os.makedirs(out_dir, exist_ok=True)
    manifest = {
        "author": AUTHOR,
        "tool": TOOL_NAME,
        "tool_sha256": sha256_file(os.path.abspath(__file__)),
        "scene": os.path.abspath(scene_path),
        "scene_sha256": sha256_file(scene_path),
        "scale": scale,
        "meshes": mesh_records,
        "views": [],
    }
    for view in views:
        start = time.perf_counter()
        packed, vs = render_view(view, batches, bbox, lighting, background, W, H, ground_cfg)
        png = encode_png(packed, W, H)
        png_name = view["name"] + ".png"
        with open(os.path.join(out_dir, png_name), "wb") as fh:
            fh.write(png)
        elapsed = time.perf_counter() - start
        record = dict(view)
        record.update({
            "width": W,
            "height": H,
            "png": png_name,
            "png_sha256": hashlib.sha256(png).hexdigest(),
            "elapsed_s": round(elapsed, 3),
            "camera": {
                "position": [round(c, 3) for c in vs.position],
                "forward": [round(c, 6) for c in vs.forward],
                "right": [round(c, 6) for c in vs.right],
                "up": [round(c, 6) for c in vs.up],
                "target": list(vs.target),
                "width_cm": round(vs.width_cm, 3),
                "height_cm": round(vs.height_cm, 3),
            },
        })
        manifest["views"].append(record)
        if log:
            log(f"{view['name']}: {W}x{H} {elapsed:.2f}s -> {png_name}")
    with open(os.path.join(out_dir, "render-manifest.json"), "w", encoding="utf-8") as fh:
        json.dump(manifest, fh, indent=2)
        fh.write("\n")
    return manifest


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog=TOOL_NAME,
        description="Render review views (ortho + Unreal-style tactical perspective) of OBJ blockouts.",
    )
    parser.add_argument("--scene", required=True, help="scene JSON path")
    parser.add_argument("--out", required=True, help="output directory")
    parser.add_argument("--scale", type=float, default=1.0, help="resolution multiplier (default 1.0)")
    parser.add_argument("--views", default=None, help="comma-separated subset of view names")
    args = parser.parse_args(argv)
    names = [n.strip() for n in args.views.split(",") if n.strip()] if args.views else None
    try:
        render_scene(args.scene, args.out, args.scale, names, log=lambda m: print(m, file=sys.stderr))
    except RenderError as exc:
        print(f"{TOOL_NAME}: error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
