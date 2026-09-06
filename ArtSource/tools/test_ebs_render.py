#!/usr/bin/env python3
"""Tests for ebs_render.py (standard library unittest only).

Builds a synthetic scene in a temp dir: a 100 cm cube centered at (0,0,50)
whose +X face is MI_Front (red), a second cube offset to y=+200 (MI_Left,
blue), and a 300 cm post at (+300, 0) (MI_Post, green). Verifies view
conventions, camera math, PNG integrity, determinism and grayscale output.

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

    def test_perspective_views_render(self):
        W, H, px = self._view("tactical_default")
        self.assertIsNotNone(_centroid(px, GREEN), "post missing in tactical view")
        bg = 0x1C1F24
        non_bg = sum(1 for row in px for c in row if c != bg)
        self.assertGreater(non_bg, W * H // 4, "ground plane should fill a large part of the tactical view")

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
        self.assertEqual(len(m["views"]), 7)
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
