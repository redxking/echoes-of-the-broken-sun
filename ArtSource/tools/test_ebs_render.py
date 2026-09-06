#!/usr/bin/env python3
"""Tests for ebs_render.py (standard library unittest only).

Builds a synthetic scene in a temp dir: a 100 cm cube centered at (0,0,50)
whose +X face is MI_Front (red), a second cube offset to y=+200 (MI_Left,
blue), and a 300 cm post at (+300, 0) (MI_Post, green). Verifies view
conventions, camera math, PNG integrity, determinism and grayscale output.

Tactical views (type "persp") are orthographic by default, matching
AEchoesRTSCameraPawn (ProjectionMode Orthographic, OrthoWidth =
2 * arm * tan(fov / 2)) with the engine-default MaintainYFOV axis constraint
(OrthoWidth spans the image height). DefectRegressionTest pins the fixes for
the verifier's findings: ortho tactical projection and frame axis, near-plane
clipping, slope-scaled edge bias, numeric-field validation, emissive
validation, mesh scale in the manifest and image_up on side views.

Run: python3 -m unittest test_ebs_render   or   python3 test_ebs_render.py

Author: Angelis Pseftis
"""
from __future__ import annotations

import json
import math
import os
import shutil
import struct
import sys
import tempfile
import unittest
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import ebs_render  # noqa: E402

RED = 0xFF0000
GREEN = 0x00FF00
BLUE = 0x0000FF
TEST_SCALE = 0.5  # 960x540 keeps the suite fast; full-res timing is a separate run


def _box_obj(lines, cx, cy, cz, sx, sy, sz, front_material, body_material, base):
    hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
    corners = [
        (cx - hx, cy - hy, cz - hz), (cx + hx, cy - hy, cz - hz),
        (cx + hx, cy + hy, cz - hz), (cx - hx, cy + hy, cz - hz),
        (cx - hx, cy - hy, cz + hz), (cx + hx, cy - hy, cz + hz),
        (cx + hx, cy + hy, cz + hz), (cx - hx, cy + hy, cz + hz),
    ]
    for v in corners:
        lines.append("v %.3f %.3f %.3f" % v)
    # +X face is (1, 2, 6, 5) in local indices
    faces = {
        "px": (1, 2, 6, 5), "nx": (3, 0, 4, 7), "py": (2, 3, 7, 6),
        "ny": (0, 1, 5, 4), "pz": (4, 5, 6, 7), "nz": (0, 3, 2, 1),
    }
    lines.append("usemtl " + front_material)
    lines.append("f " + " ".join(str(base + i + 1) for i in faces["px"]))
    lines.append("usemtl " + body_material)
    for name in ("nx", "py", "ny", "pz", "nz"):
        lines.append("f " + " ".join("%d//1" % (base + i + 1) for i in faces[name]))
    return base + 8


def write_synthetic_scene(root, width=1920, height=1080):
    lines = ["# synthetic blockout for ebs_render tests", "o cube", "vn 0 0 1"]
    base = 0
    base = _box_obj(lines, 0, 0, 50, 100, 100, 100, "MI_Front", "MI_Body", base)
    lines.append("o cube_left")
    base = _box_obj(lines, 0, 200, 50, 100, 100, 100, "MI_Left", "MI_Left", base)
    lines.append("g post")
    lines.append("usemtl MI_Post")
    # thin 20x20x300 post at (+300, 0): written with negative indices and i/t/n tokens
    px, py = 300.0, 0.0
    post = [
        (px - 10, py - 10, 0), (px + 10, py - 10, 0), (px + 10, py + 10, 0), (px - 10, py + 10, 0),
        (px - 10, py - 10, 300), (px + 10, py - 10, 300), (px + 10, py + 10, 300), (px - 10, py + 10, 300),
    ]
    lines.append("vt 0 0")
    for v in post:
        lines.append("v %.1f %.1f %.1f" % v)
    quads = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    for q in quads:
        lines.append("f " + " ".join("%d/1/1" % (i - 8) for i in q))
    obj_path = os.path.join(root, "synthetic.obj")
    with open(obj_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    scene = {
        "meshes": [{"obj": "synthetic.obj", "translate": [0, 0, 0], "yaw_deg": 0}],
        "materials": {
            "MI_Front": [1, 0, 0], "MI_Body": [0.6, 0.6, 0.6], "MI_Left": [0, 0, 1],
            "MI_Post": [0, 1, 0], "_default": [0.6, 0.6, 0.6],
        },
        "emissive": ["MI_Front", "MI_Left", "MI_Post"],
        "width": width, "height": height,
        "background": [0.11, 0.12, 0.14],
        "light": {"direction": [-0.45, 0.35, -0.82], "ambient": 0.35, "key": 0.75,
                  "color": [1.0, 0.82, 0.62], "fill_color": [0.48, 0.60, 0.88], "fill": 0.25},
        "ground": {"tile_cm": 200, "tiles": 12, "color": [0.22, 0.23, 0.25],
                   "grid_color": [0.34, 0.36, 0.40], "footprint_cm": [400, 400],
                   "footprint_color": [0.95, 0.62, 0.18]},
        "reference_figure": {"height_cm": 180, "position": [-300, -300, 0], "color": [0.5, 0.45, 0.4]},
        "views": [
            {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15},
            {"name": "right", "type": "ortho", "from": "+Y"},
            {"name": "rear", "type": "ortho", "from": "-X"},
            {"name": "left", "type": "ortho", "from": "-Y"},
            {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True},
            {"name": "tactical_default", "type": "persp", "pitch_deg": -48, "yaw_deg": -45,
             "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0], "edges": True},
            {"name": "tactical_mono", "type": "persp", "pitch_deg": -60, "yaw_deg": -45,
             "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0], "grayscale": True},
            {"name": "tactical_persp", "type": "persp", "projection": "perspective", "pitch_deg": -48,
             "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0], "edges": True},
        ],
    }
    scene_path = os.path.join(root, "scene.json")
    with open(scene_path, "w", encoding="utf-8") as fh:
        json.dump(scene, fh, indent=1)
    return scene_path


def _parse_png(data):
    """Independent chunk walk (not the tool's decoder): returns (W, H, raw rows)."""
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos = 8
    idat = b""
    W = H = None
    seen = []
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        crc = struct.unpack(">I", data[pos + 8 + length:pos + 12 + length])[0]
        assert zlib.crc32(tag + payload) & 0xFFFFFFFF == crc, "CRC mismatch in %r" % tag
        seen.append(tag)
        if tag == b"IHDR":
            W, H = struct.unpack(">II", payload[:8])
            assert payload[8] == 8 and payload[9] == 2
        elif tag == b"IDAT":
            idat += payload
        pos += 12 + length
    assert seen[0] == b"IHDR" and seen[-1] == b"IEND"
    raw = zlib.decompress(idat)
    return W, H, raw


def _pixels(path):
    with open(path, "rb") as fh:
        data = fh.read()
    W, H, raw = _parse_png(data)
    stride = 1 + 3 * W
    assert len(raw) == stride * H
    px = []
    for y in range(H):
        assert raw[y * stride] == 0
        row = raw[y * stride + 1:(y + 1) * stride]
        px.append([(row[3 * x] << 16) | (row[3 * x + 1] << 8) | row[3 * x + 2] for x in range(W)])
    return W, H, px


def _centroid(px, color):
    xs = ys = n = 0
    for y, row in enumerate(px):
        for x, c in enumerate(row):
            if c == color:
                xs += x
                ys += y
                n += 1
    if n == 0:
        return None
    return xs / n, ys / n, n


class SyntheticSceneTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = tempfile.mkdtemp(prefix="ebs_render_test_")
        cls.scene_path = write_synthetic_scene(cls.root)
        cls.out = os.path.join(cls.root, "out")
        cls.manifest = ebs_render.render_scene(cls.scene_path, cls.out, scale=TEST_SCALE)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.root, ignore_errors=True)

    def _view(self, name):
        return _pixels(os.path.join(self.out, name + ".png"))

    def test_obj_loader_forms(self):
        verts, faces = ebs_render.load_obj(os.path.join(self.root, "synthetic.obj"))
        self.assertEqual(len(verts), 24)
        self.assertEqual(len(faces), 36)  # 3 boxes x 6 quads x 2 triangles
        mats = {f[3] for f in faces}
        self.assertEqual(mats, {"MI_Front", "MI_Body", "MI_Left", "MI_Post"})
        self.assertEqual(sum(1 for f in faces if f[3] == "MI_Post"), 12)

    def test_front_view_convention(self):
        W, H, px = self._view("front")
        # red +X face visible around the image center (cube spans z 0..100 above center row)
        reds = 0
        for y in range(H // 2 - H // 8, H // 2):
            for x in range(W // 2 - W // 12, W // 2 + W // 12):
                if px[y][x] == RED:
                    reds += 1
        self.assertGreater(reds, 50, "red +X face not visible in front view center region")
        # post at (+300, 0) is straight ahead: centered horizontally
        post = _centroid(px, GREEN)
        self.assertIsNotNone(post, "post not visible in front view")
        self.assertLess(abs(post[0] - W / 2), 3.0, "post is not horizontally centered: %r" % (post,))
        # camera at +X: right = up x forward = (0,-1,0); world +Y appears on image LEFT
        left_cube = _centroid(px, BLUE)
        self.assertIsNotNone(left_cube, "y=+200 cube not visible in front view")
        self.assertLess(left_cube[0], W / 2 - 40, "y=+200 cube should be left of center")
        # the +X face of the offset cube is not red, so no red should be on the far left
        self.assertGreater(left_cube[2], reds // 4)

    def test_top_view_image_up(self):
        W, H, px = self._view("top")
        post = _centroid(px, GREEN)
        self.assertIsNotNone(post, "post not visible in top view")
        self.assertLess(post[1], H / 2 - 40, "+X offset object should be above center with image_up +X")
        self.assertLess(abs(post[0] - W / 2), 3.0)
        # +Y appears on image right in the top view (right = (+X) x (-Z) = (0,1,0))
        left_cube = _centroid(px, BLUE)
        self.assertIsNotNone(left_cube)
        self.assertGreater(left_cube[0], W / 2 + 40)

    def test_side_views_render_post(self):
        for name in ("right", "rear", "left"):
            W, H, px = self._view(name)
            post = _centroid(px, GREEN)
            self.assertIsNotNone(post, "%s: post missing" % name)
        # from +Y: right = (0,0,1) x (0,-1,0) = (1,0,0): +X appears on the image RIGHT
        W, H, px = self._view("right")
        self.assertGreater(_centroid(px, GREEN)[0], W / 2 + 40)
        # from -Y: +X appears on the image LEFT
        W, H, px = self._view("left")
        self.assertLess(_centroid(px, GREEN)[0], W / 2 - 40)
        # rear: red face hidden behind the cube body
        W, H, px = self._view("rear")
        self.assertIsNone(_centroid(px, RED))

    def test_perspective_camera_position(self):
        pos, forward, right, up = ebs_render.perspective_camera(-48.0, -45.0, 3800.0, (0.0, 0.0, 0.0))
        p, y = math.radians(-48.0), math.radians(-45.0)
        expected_forward = (math.cos(p) * math.cos(y), math.cos(p) * math.sin(y), math.sin(p))
        expected = tuple(-c * 3800.0 for c in expected_forward)
        for a, b in zip(pos, expected):
            self.assertLess(abs(a - b), 0.5)
        self.assertLess(abs(pos[0] - (-1798.1)), 0.5)
        self.assertLess(abs(pos[1] - 1798.1), 0.5)
        self.assertLess(abs(pos[2] - 2824.0), 0.5)
        # basis identities in the left-handed frame
        self.assertLess(abs(ebs_render.v_dot(forward, right)), 1e-9)
        self.assertLess(abs(ebs_render.v_dot(forward, up)), 1e-9)
        self.assertGreater(up[2], 0.0)
        self.assertEqual(ebs_render.v_cross((0, 0, 1), (1, 0, 0)), (0, 1, 0))
        # manifest carries the same position
        rec = next(v for v in self.manifest["views"] if v["name"] == "tactical_default")
        for a, b in zip(rec["camera"]["position"], expected):
            self.assertLess(abs(a - b), 0.5)

    def test_tactical_views_render(self):
        bg = 0x1C1F24
        W, H, px = self._view("tactical_default")
        self.assertIsNotNone(_centroid(px, GREEN), "post missing in tactical view")
        non_bg = sum(1 for row in px for c in row if c != bg)
        # ortho tactical frame is 7033 x 3956 cm; the 2400 cm ground covers a modest part of it
        self.assertGreater(non_bg, W * H // 12, "ground plane should be visible in the tactical view")
        rec = next(v for v in self.manifest["views"] if v["name"] == "tactical_default")
        self.assertEqual(rec["camera"]["projection"], "ortho")
        self.assertEqual(rec["camera"]["frame_axis"], "height")
        # legacy perspective path still renders and its ground fills most of the frame
        W, H, px = self._view("tactical_persp")
        self.assertIsNotNone(_centroid(px, GREEN), "post missing in legacy perspective view")
        non_bg = sum(1 for row in px for c in row if c != bg)
        self.assertGreater(non_bg, W * H // 4)
        rec = next(v for v in self.manifest["views"] if v["name"] == "tactical_persp")
        self.assertEqual(rec["camera"]["projection"], "perspective")
        self.assertAlmostEqual(rec["camera"]["width_cm"], 2 * 3800 * math.tan(math.radians(27.5)), places=2)

    def test_png_decodes(self):
        for rec in self.manifest["views"]:
            path = os.path.join(self.out, rec["png"])
            with open(path, "rb") as fh:
                data = fh.read()
            W, H, raw = _parse_png(data)
            self.assertEqual((W, H), (rec["width"], rec["height"]))
            self.assertEqual((W, H), (960, 540))
            self.assertEqual(len(raw), H * (1 + 3 * W))
            self.assertEqual(rec["png_sha256"], ebs_render.hashlib.sha256(data).hexdigest())
            dw, dh, _ = ebs_render.decode_png(data)
            self.assertEqual((dw, dh), (W, H))

    def test_deterministic_bytes(self):
        out2 = os.path.join(self.root, "out2")
        ebs_render.render_scene(self.scene_path, out2, scale=TEST_SCALE, view_names=["front", "tactical_mono"])
        for name in ("front", "tactical_mono"):
            with open(os.path.join(self.out, name + ".png"), "rb") as a, \
                    open(os.path.join(out2, name + ".png"), "rb") as b:
                self.assertEqual(a.read(), b.read(), "%s.png differs between runs" % name)
        with open(os.path.join(out2, "render-manifest.json"), encoding="utf-8") as fh:
            m2 = json.load(fh)
        self.assertEqual([v["name"] for v in m2["views"]], ["front", "tactical_mono"])

    def test_grayscale_output(self):
        W, H, px = self._view("tactical_mono")
        distinct = set()
        for row in px:
            for c in row:
                r, g, b = c >> 16, (c >> 8) & 255, c & 255
                self.assertEqual(r, g)
                self.assertEqual(g, b)
                distinct.add(c)
        self.assertGreater(len(distinct), 3)

    def test_manifest_fields(self):
        m = self.manifest
        self.assertEqual(m["author"], "Angelis Pseftis")
        self.assertEqual(m["tool"], "ebs_render.py")
        self.assertEqual(len(m["tool_sha256"]), 64)
        self.assertEqual(len(m["scene_sha256"]), 64)
        self.assertEqual(m["meshes"][0]["triangles"], 36)
        self.assertEqual(len(m["meshes"][0]["sha256"]), 64)
        self.assertEqual(len(m["views"]), 8)
        self.assertEqual(m["meshes"][0]["scale"], 1.0)
        with open(os.path.join(self.out, "render-manifest.json"), encoding="utf-8") as fh:
            on_disk = json.load(fh)
        self.assertEqual(on_disk["views"][0]["png"], "front.png")
        self.assertIn("elapsed_s", on_disk["views"][0])

    def test_ortho_width_override(self):
        out3 = os.path.join(self.root, "out3")
        with open(self.scene_path, encoding="utf-8") as fh:
            scene = json.load(fh)
        scene["views"] = [{"name": "front_fixed", "type": "ortho", "from": "+X", "ortho_width_cm": 2000}]
        path = os.path.join(self.root, "scene_fixed.json")
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(scene, fh)
        m = ebs_render.render_scene(path, out3, scale=0.25)
        self.assertEqual(m["views"][0]["camera"]["width_cm"], 2000.0)
        W, H, px = _pixels(os.path.join(out3, "front_fixed.png"))
        post = _centroid(px, GREEN)
        self.assertIsNotNone(post)
        # 20 cm post over 2000 cm -> ~1% of width
        self.assertLess(post[2] / (W * H), 0.02)


EDGE = 0x0D0D12  # default edge color (0.05, 0.05, 0.07)


def _bbox(px, color):
    xs = [x for row in px for x, c in enumerate(row) if c == color]
    ys = [y for y, row in enumerate(px) for c in row if c == color]
    if not xs:
        return None
    return min(xs), min(ys), max(xs), max(ys)


def _box_lines(lines, cx, cy, cz, sx, sy, sz, material, base):
    """All six faces in one material (uses the test helper's face table)."""
    return _box_obj(lines, cx, cy, cz, sx, sy, sz, material, material, base)


class DefectRegressionTest(unittest.TestCase):
    """One test per verifier finding; small scenes rendered through the API."""

    def setUp(self):
        self.root = tempfile.mkdtemp(prefix="ebs_render_regress_")

    def tearDown(self):
        shutil.rmtree(self.root, ignore_errors=True)

    def _scene(self, name, lines, materials, views, width=480, height=270, extra=None):
        obj = os.path.join(self.root, name + ".obj")
        with open(obj, "w", encoding="utf-8") as fh:
            fh.write("\n".join(lines) + "\n")
        scene = {"meshes": [{"obj": name + ".obj"}], "materials": materials,
                 "emissive": list(materials), "width": width, "height": height,
                 "background": [0, 0, 0], "views": views}
        if extra:
            scene.update(extra)
        path = os.path.join(self.root, name + ".json")
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(scene, fh)
        return path

    TACTICAL = {"pitch_deg": -48, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]}

    def test_tactical_default_is_orthographic(self):
        # setup: type "persp" -> orthographic camera with the game's EquivalentOrthoWidth
        view = dict(self.TACTICAL, name="t", type="persp")
        vs = ebs_render.setup_view(view, None, 1920, 1080)
        extent = 2.0 * 3800.0 * math.tan(math.radians(55.0) * 0.5)
        self.assertEqual(vs.kind, "ortho")
        self.assertEqual(vs.projection, "ortho")
        self.assertAlmostEqual(vs.height_cm, extent, places=6)
        self.assertAlmostEqual(vs.width_cm, extent * 1920.0 / 1080.0, places=6)
        self.assertAlmostEqual(extent, 3956.31, delta=0.01)  # EquivalentOrthoWidth for the 3800/55 preset
        # spring-arm basis and position are unchanged by the projection choice
        pos, fwd, right, up = ebs_render.perspective_camera(-48.0, -45.0, 3800.0, (0.0, 0.0, 0.0))
        self.assertEqual(vs.position, pos)
        self.assertEqual(vs.forward, fwd)
        # pixels: two identical slabs, one near the camera and one far, have the same screen width
        # (screen-horizontal extent scales exactly with 1/depth in perspective and not at all in ortho)
        lines = ["o slabs"]
        # (-600, 600) is 568 cm nearer the camera along its forward vector, (600, -600) 568 cm farther
        base = _box_lines(lines, -600, 600, 25, 200, 200, 50, "NEAR", 0)
        _box_lines(lines, 600, -600, 25, 200, 200, 50, "FAR", base)
        views = [dict(self.TACTICAL, name="ortho", type="persp"),
                 dict(self.TACTICAL, name="persp", type="persp", projection="perspective")]
        path = self._scene("posts", lines, {"NEAR": [0, 1, 0], "FAR": [0, 0, 1]}, views, 960, 540)
        m = ebs_render.render_scene(path, os.path.join(self.root, "o"))
        widths = {}
        for rec in m["views"]:
            W, H, px = _pixels(os.path.join(self.root, "o", rec["png"]))
            near, far = _bbox(px, GREEN), _bbox(px, BLUE)
            self.assertIsNotNone(near)
            self.assertIsNotNone(far)
            widths[rec["name"]] = (near[2] - near[0], far[2] - far[0])
        n, f = widths["ortho"]
        self.assertGreater(n, 20)
        self.assertLessEqual(abs(n - f), 2, "ortho tactical: near/far slabs must match: %r" % (widths,))
        n, f = widths["persp"]
        # depth ratio (3800 + 568) / (3800 - 568) = 1.35
        self.assertGreater(n, f * 1.2, "legacy perspective: near slab must be wider: %r" % (widths,))

    def test_tactical_frame_axis(self):
        extent = 2.0 * 3800.0 * math.tan(math.radians(55.0) * 0.5)
        vs = ebs_render.setup_view(dict(self.TACTICAL, name="t", type="persp", frame_axis="width"), None, 1920, 1080)
        self.assertAlmostEqual(vs.width_cm, extent, places=6)
        self.assertAlmostEqual(vs.height_cm, extent * 1080.0 / 1920.0, places=6)
        self.assertEqual(vs.frame_axis, "width")
        vs = ebs_render.setup_view(dict(self.TACTICAL, name="t", type="persp"), None, 1920, 1080)
        self.assertEqual(vs.frame_axis, "height")
        with self.assertRaises(ebs_render.RenderError):
            ebs_render.setup_view(dict(self.TACTICAL, name="t", type="persp", frame_axis="diagonal"), None, 16, 9)
        with self.assertRaises(ebs_render.RenderError):
            ebs_render.setup_view(dict(self.TACTICAL, name="t", type="persp", projection="fisheye"), None, 16, 9)
        # frame_axis is recorded in the manifest
        path = self._scene("fa", ["v 0 0 0", "v 100 0 0", "v 0 100 0", "usemtl P", "f 1 2 3"], {"P": [1, 0, 0]},
                           [dict(self.TACTICAL, name="w", type="persp", frame_axis="width")], 64, 36)
        m = ebs_render.render_scene(path, os.path.join(self.root, "o"))
        self.assertEqual(m["views"][0]["camera"]["frame_axis"], "width")
        self.assertAlmostEqual(m["views"][0]["camera"]["width_cm"], round(extent, 3), places=3)

    def test_near_plane_clipping(self):
        # a single 10000 cm plate quad has a corner behind the legacy perspective camera
        plate = ["o plate", "v -5000 -5000 0", "v 5000 -5000 0", "v 5000 5000 0", "v -5000 5000 0",
                 "usemtl PLATE", "f 1 2 3 4"]
        grid = ["o grid"]
        n = 20
        step = 10000.0 / n
        for iy in range(n + 1):
            for ix in range(n + 1):
                grid.append("v %.1f %.1f 0" % (-5000 + ix * step, -5000 + iy * step))
        grid.append("usemtl PLATE")
        for iy in range(n):
            for ix in range(n):
                a = iy * (n + 1) + ix + 1
                grid.append("f %d %d %d %d" % (a, a + 1, a + n + 2, a + n + 1))
        view = dict(self.TACTICAL, name="tac", type="persp", projection="perspective", edges=True)
        counts = {}
        for name, lines in (("plate", plate), ("grid", grid)):
            path = self._scene(name, lines, {"PLATE": [0, 1, 0]}, [view])
            ebs_render.render_scene(path, os.path.join(self.root, "o_" + name))
            W, H, px = _pixels(os.path.join(self.root, "o_" + name, "tac.png"))
            counts[name] = sum(1 for row in px for c in row if c == GREEN)
            self.assertEqual(px[H - 1][W // 2], GREEN, "%s: bottom-centre pixel must be plate" % name)
            self.assertEqual(px[H - 1][0], GREEN)
            self.assertEqual(px[H - 1][W - 1], GREEN)
        self.assertGreater(counts["plate"], W * H * 0.9)
        self.assertLess(abs(counts["plate"] - counts["grid"]), W * H * 0.005,
                        "clipped plate must cover the same pixels as a pre-subdivided one: %r" % (counts,))
        # camera-space clipper: one vertex behind the near plane yields a quad, two yield a triangle
        proj = lambda p: p
        near = ebs_render.NEAR_PLANE_CM
        poly = ebs_render._clip_triangle_near((0, 0, 10), (1, 0, 10), (0, 1, -10), proj)
        self.assertEqual(len(poly), 4)
        self.assertTrue(all(p[2] >= near for p in poly))
        poly = ebs_render._clip_triangle_near((0, 0, 10), (1, 0, -10), (0, 1, -10), proj)
        self.assertEqual(len(poly), 3)
        self.assertEqual(ebs_render._clip_triangle_near((0, 0, -1), (1, 0, -5), (0, 1, -10), proj), [])
        seg = ebs_render._clip_edge_near((0, 0, 10), (0, 0, -10))
        self.assertEqual(seg[1][2], near)
        self.assertIsNone(ebs_render._clip_edge_near((0, 0, 0), (0, 0, -10)))

    def test_hidden_edges_stay_hidden_in_wide_frames(self):
        # 2 cm wall with a cube 6 cm behind its front face, ortho from +X at 4000 cm width
        lines = ["o wall"]
        base = _box_lines(lines, 0, 0, 150, 2, 600, 300, "WALL", 0)
        _box_lines(lines, -1 - 6 - 25, 0, 150, 50, 50, 50, "HID", base)
        views = [{"name": "w%d" % w, "type": "ortho", "from": "+X", "edges": True, "ortho_width_cm": w}
                 for w in (800, 4000, 8000)]
        views.append(dict(self.TACTICAL, name="tac", type="persp", edges=True, target=[0, 0, 150]))
        path = self._scene("wall", lines, {"WALL": [1, 0, 0], "HID": [0, 0, 1]}, views, 1920, 1080)
        m = ebs_render.render_scene(path, os.path.join(self.root, "o"))
        for rec in m["views"]:
            if rec["name"] == "tac":
                continue
            W, H, px = _pixels(os.path.join(self.root, "o", rec["png"]))
            bb = _bbox(px, RED)
            self.assertIsNotNone(bb)
            inner = [(x, y) for y in range(bb[1] + 4, bb[3] - 3) for x in range(bb[0] + 4, bb[2] - 3)
                     if px[y][x] == EDGE]
            self.assertEqual(inner, [], "%s: hidden cube edges leak through the wall" % rec["name"])
            self.assertEqual(sum(1 for row in px for c in row if c == BLUE), 0)
            # the wall's own outline is still drawn (at least one full horizontal edge)
            self.assertGreater(sum(1 for row in px for c in row if c == EDGE), bb[2] - bb[0])
        # the bias is now a small fraction of the extent plus a slope term
        self.assertLessEqual(ebs_render.EDGE_BIAS_FRACTION, 0.0005)
        vs = ebs_render.setup_view(views[1], None, 1920, 1080)
        self.assertAlmostEqual(vs.bias_c, 0.0005 * 4000.0)

    def test_visible_edges_complete(self):
        lines = ["o cube"]
        _box_lines(lines, 0, 0, 50, 100, 100, 100, "C", 0)
        views = [dict(self.TACTICAL, name="ortho", type="persp", edges=True, arm_cm=900, target=[0, 0, 50]),
                 dict(self.TACTICAL, name="persp", type="persp", projection="perspective", edges=True,
                      arm_cm=900, target=[0, 0, 50])]
        path = self._scene("cube", lines, {"C": [1, 0, 0]}, views, 800, 600)
        ebs_render.render_scene(path, os.path.join(self.root, "o"))
        verts, _ = ebs_render.load_obj(os.path.join(self.root, "cube.obj"))
        faces = {"px": (1, 2, 6, 5), "nx": (3, 0, 4, 7), "py": (2, 3, 7, 6),
                 "ny": (0, 1, 5, 4), "pz": (4, 5, 6, 7), "nz": (0, 3, 2, 1)}
        for view in views:
            vs = ebs_render.setup_view(view, None, 800, 600)
            PX, PY, PD, OK, CS = ebs_render._project(vs, verts, 800, 600)
            W, H, px = _pixels(os.path.join(self.root, "o", view["name"] + ".png"))
            missing = total = 0
            for q in faces.values():
                a, b, c = verts[q[0]], verts[q[1]], verts[q[2]]
                n = ebs_render.v_cross(ebs_render.v_sub(b, a), ebs_render.v_sub(c, a))
                cen = tuple(sum(verts[i][k] for i in q) / 4 for k in range(3))
                if vs.kind == "persp":
                    facing = ebs_render.v_dot(n, ebs_render.v_sub(vs.position, cen))
                else:
                    facing = -ebs_render.v_dot(n, vs.forward)
                if facing <= 0:
                    continue
                for i in range(4):
                    e0, e1 = q[i], q[(i + 1) % 4]
                    for t in [k / 20 for k in range(1, 20)]:
                        x = PX[e0] + (PX[e1] - PX[e0]) * t
                        y = PY[e0] + (PY[e1] - PY[e0]) * t
                        total += 1
                        if not any(px[int(y) + dy][int(x) + dx] == EDGE for dy in (-1, 0, 1) for dx in (-1, 0, 1)):
                            missing += 1
            self.assertEqual(missing, 0, "%s: %d of %d visible-edge samples have no line pixel"
                             % (view["name"], missing, total))
            self.assertGreater(total, 100)

    def test_non_numeric_fields_are_render_errors(self):
        scene_path = write_synthetic_scene(self.root)
        with open(scene_path, encoding="utf-8") as fh:
            base = json.load(fh)
        cases = [
            ("margin", lambda s: s["views"][0].update({"margin": "big"})),
            ("pitch_deg", lambda s: s["views"][5].update({"pitch_deg": "steep"})),
            ("arm_cm", lambda s: s["views"][5].update({"arm_cm": None})),
            ("width", lambda s: s.update({"width": "wide"})),
            ("height", lambda s: s.update({"height": 10.5})),
            ("scale", lambda s: s["meshes"][0].update({"scale": "x"})),
            ("yaw_deg", lambda s: s["meshes"][0].update({"yaw_deg": True})),
            ("tile_cm", lambda s: s["ground"].update({"tile_cm": "200"})),
            ("tiles", lambda s: s["ground"].update({"tiles": "12"})),
            ("height_cm", lambda s: s["reference_figure"].update({"height_cm": [180]})),
            ("ambient", lambda s: s["light"].update({"ambient": "dim"})),
            ("ortho_width_cm", lambda s: s["views"][1].update({"ortho_width_cm": "wide"})),
        ]
        for label, mutate in cases:
            scene = json.loads(json.dumps(base))
            mutate(scene)
            path = os.path.join(self.root, "bad_%s.json" % label)
            with open(path, "w", encoding="utf-8") as fh:
                json.dump(scene, fh)
            with self.assertRaises(ebs_render.RenderError, msg=label):
                ebs_render.render_scene(path, os.path.join(self.root, "o"), scale=0.05)
            self.assertEqual(ebs_render.main(["--scene", path, "--out", os.path.join(self.root, "o"),
                                              "--scale", "0.05"]), 2, label)

    def test_emissive_must_be_a_list(self):
        scene_path = write_synthetic_scene(self.root)
        with open(scene_path, encoding="utf-8") as fh:
            scene = json.load(fh)
        for bad in ("MI_Front", {"MI_Front": 1}, ["MI_Front", 3]):
            scene["emissive"] = bad
            path = os.path.join(self.root, "emissive.json")
            with open(path, "w", encoding="utf-8") as fh:
                json.dump(scene, fh)
            with self.assertRaises(ebs_render.RenderError):
                ebs_render.render_scene(path, os.path.join(self.root, "o"), scale=0.05)
        scene["emissive"] = None
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(scene, fh)
        ebs_render.render_scene(path, os.path.join(self.root, "o"), scale=0.05, view_names=["front"])

    def test_manifest_records_mesh_scale(self):
        scene_path = write_synthetic_scene(self.root)
        with open(scene_path, encoding="utf-8") as fh:
            scene = json.load(fh)
        scene["meshes"][0]["scale"] = 2.0
        scene["views"] = [{"name": "front", "type": "ortho", "from": "+X", "ortho_width_cm": 2000}]
        path = os.path.join(self.root, "scaled.json")
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(scene, fh)
        m = ebs_render.render_scene(path, os.path.join(self.root, "o2"), scale=0.25)
        self.assertEqual(m["meshes"][0]["scale"], 2.0)
        self.assertEqual(sorted(m["meshes"][0]), ["obj", "scale", "sha256", "translate", "triangles", "yaw_deg"])
        scene["meshes"][0]["scale"] = 1.0
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(scene, fh)
        m1 = ebs_render.render_scene(path, os.path.join(self.root, "o1"), scale=0.25)
        self.assertEqual(m1["meshes"][0]["scale"], 1.0)
        W, H, px2 = _pixels(os.path.join(self.root, "o2", "front.png"))
        W, H, px1 = _pixels(os.path.join(self.root, "o1", "front.png"))
        post2 = _centroid(px2, GREEN)
        post1 = _centroid(px1, GREEN)
        self.assertGreater(post2[2], post1[2] * 3.0, "scale 2 post should cover ~4x the pixels")
        scene["meshes"][0]["scale"] = 0
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(scene, fh)
        with self.assertRaises(ebs_render.RenderError):
            ebs_render.render_scene(path, os.path.join(self.root, "o0"), scale=0.25)

    def test_image_up_honoured_on_side_views(self):
        default = ebs_render.ortho_axes("+X")
        self.assertEqual(default, ebs_render.ortho_axes("+X", "+Z"))
        self.assertEqual(default[2], (0.0, 0.0, 1.0))
        rolled = ebs_render.ortho_axes("+X", "+Y")
        self.assertNotEqual(rolled, default)
        self.assertEqual(rolled[2], (0.0, 1.0, 0.0))
        self.assertEqual(rolled[1], (0.0, 0.0, 1.0))  # right = up x forward = (0,1,0) x (-1,0,0)
        with self.assertRaises(ebs_render.RenderError):
            ebs_render.ortho_axes("+X", "+X")
        with self.assertRaises(ebs_render.RenderError):
            ebs_render.ortho_axes("-Y", "-Y")
        with self.assertRaises(ebs_render.RenderError):
            ebs_render.ortho_axes("+Y", "up")
        # top view default is unchanged
        self.assertEqual(ebs_render.ortho_axes("+Z"), ebs_render.ortho_axes("+Z", "+X"))
        # rendered: front view with image_up +Y puts the +Y-offset cube ABOVE centre instead of left
        scene_path = write_synthetic_scene(self.root)
        with open(scene_path, encoding="utf-8") as fh:
            scene = json.load(fh)
        scene["views"] = [{"name": "front_rolled", "type": "ortho", "from": "+X", "image_up": "+Y"}]
        path = os.path.join(self.root, "rolled.json")
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(scene, fh)
        ebs_render.render_scene(path, os.path.join(self.root, "o"), scale=0.25)
        W, H, px = _pixels(os.path.join(self.root, "o", "front_rolled.png"))
        cube = _centroid(px, BLUE)
        self.assertIsNotNone(cube)
        self.assertLess(cube[1], H / 2 - 10)
        self.assertLess(abs(cube[0] - W / 2), W / 6)


class BadInputTest(unittest.TestCase):
    def test_missing_scene(self):
        with self.assertRaises(ebs_render.RenderError):
            ebs_render.render_scene("/nonexistent/scene.json", tempfile.mkdtemp())

    def test_unknown_view_and_bad_obj(self):
        root = tempfile.mkdtemp(prefix="ebs_render_bad_")
        try:
            scene_path = write_synthetic_scene(root)
            with self.assertRaises(ebs_render.RenderError):
                ebs_render.render_scene(scene_path, os.path.join(root, "o"), view_names=["nope"])
            bad = os.path.join(root, "bad.obj")
            with open(bad, "w") as fh:
                fh.write("v 0 0 0\nv 1 0 0\nf 1 2 9\n")
            with self.assertRaises(ebs_render.RenderError):
                ebs_render.load_obj(bad)
            self.assertEqual(ebs_render.main(["--scene", scene_path, "--out", os.path.join(root, "o"),
                                              "--views", "nope"]), 2)
        finally:
            shutil.rmtree(root, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
