#!/usr/bin/env python3
"""Unit tests for ebs_texbake.py (synthetic manifests, standard library only).

Author: Angelis Pseftis

Run:
    python3 -m unittest ArtSource/tools/test_ebs_texbake.py -v
"""
from __future__ import annotations

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import ebs_texbake as tb  # noqa: E402

ATLAS = 128
DENSITY = 0.5  # px per cm -> 2 cm per px
SLOTS = {
    "MI_EBS_MER_CeramicCivic": "ceramic_civic",
    "MI_EBS_MER_CompactFrame": "compact_metal",
    "MI_EBS_MER_StatusCyan": "status_emissive",
}


def _add(a, b, s):
    return [a[0] + b[0] * s, a[1] + b[1] * s, a[2] + b[2] * s]


def make_chart(cid, component, slot, rect, origin, u_dir, v_dir, normal, cells=1, n_shift=0.0):
    x, y, w, h = rect
    cw = w // cells
    size_cm = [cw / DENSITY, h / DENSITY]
    poly_px = [(x, y), (x + cw, y), (x + cw, y + h), (x, y + h)]
    poly_uv = [[px / ATLAS, py / ATLAS] for px, py in poly_px]
    base = _add(origin, normal, n_shift)
    poly_world = [
        base,
        _add(base, u_dir, size_cm[0]),
        _add(_add(base, u_dir, size_cm[0]), v_dir, size_cm[1]),
        _add(base, v_dir, size_cm[1]),
    ]
    return {
        "id": cid,
        "component": component,
        "slot": slot,
        "meshes": ["SM_TEST:LOD0"],
        "rect_px": [x, y, w, h],
        "cell_px": [cw, h],
        "cells": cells,
        "origin_cm": list(origin),
        "u_dir": list(u_dir),
        "v_dir": list(v_dir),
        "normal": list(normal),
        "size_cm": size_cm,
        "polygon_uv": poly_uv,
        "polygon_world": poly_world,
    }


def make_manifest(charts):
    return {
        "author": "Angelis Pseftis",
        "creator": "Angelis Pseftis",
        "production_asset_id": "EBS-TEST-000",
        "revision": "test",
        "slot_families": SLOTS,
        "decal_rules": {},
        "atlas": {
            "size": ATLAS,
            "density_px_per_cm": DENSITY,
            "gutter_px": 2,
            "charts": charts,
            "used_fraction": 0.0,
        },
    }


# Chart layout of the shared synthetic atlas (128 px).
PLATE = (4, 4, 40, 40)          # ceramic panel_plate, +X face (grid markings)
WALL = (60, 4, 30, 30)          # metal plinth wall (bottom wear)
LABEL = (4, 60, 64, 9)          # 4-cell numeral strip
COLLAR = (80, 60, 20, 10)       # collar_segment_05
BAND = (4, 84, 40, 6)           # team_band
PULSE = (60, 84, 23, 4)         # conduit_pulse_strip
COUPLING = (100, 84, 10, 4)     # coupling_left status indicator


def shared_charts():
    return [
        make_chart(0, "panel_plate", "MI_EBS_MER_CeramicCivic", PLATE, [0, 40, 40], [0, -1, 0], [0, 0, -1], [1, 0, 0], n_shift=7.0),
        make_chart(1, "plinth", "MI_EBS_MER_CompactFrame", WALL, [-30, -180, 60], [1, 0, 0], [0, 0, -1], [0, -1, 0]),
        make_chart(2, "panel_label", "MI_EBS_MER_CompactFrame", LABEL, [0, -2, 84], [0, -1, 0], [0, 0, -1], [1, 0, 0], cells=4),
        make_chart(3, "collar_segment_05", "MI_EBS_MER_StatusCyan", COLLAR, [-20, 0, 928], [1, 0, 0], [0, 0, -1], [0, 1, 0]),
        make_chart(4, "team_band", "MI_EBS_MER_CeramicCivic", BAND, [0, 100, 30], [0, -1, 0], [0, 0, -1], [1, 0, 0]),
        make_chart(5, "conduit_pulse_strip", "MI_EBS_MER_StatusCyan", PULSE, [26, 0, 15], [1, 0, 0], [0, 0, -1], [0, 1, 0]),
        make_chart(6, "coupling_left", "MI_EBS_MER_StatusCyan", COUPLING, [0, -169, 93], [0, -1, 0], [0, 0, -1], [1, 0, 0]),
    ]


def px(buf, x, y, size=ATLAS):
    i = (y * size + x) * 3
    return buf[i], buf[i + 1], buf[i + 2]


class SharedBake(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = make_manifest(shared_charts())
        cls.res = tb.bake(cls.manifest, size=ATLAS)

    # (1) chart texels painted, gutters dilated
    def test_charts_painted_and_gutters_dilated(self):
        st = self.res.status
        x, y, w, h = PLATE
        self.assertEqual(st[(y + 5) * ATLAS + (x + 5)], 2)
        x, y, w, h = WALL
        self.assertEqual(st[(y + 5) * ATLAS + (x + 5)], 2)
        # gutter: gutter_px + 1 = 3 pixels beyond the polygon edge.
        right = x + w  # first pixel column outside the wall polygon
        for k in range(3):
            self.assertEqual(st[(y + 5) * ATLAS + right + k], 1, f"gutter px {k} missing")
        self.assertEqual(st[(y + 5) * ATLAS + right + 3], 0, "dilation ran past gutter_px + 1")
        # gutter pixels carry the chart's colour, not the neutral grey.
        neutral = tb.linear_to_srgb8(tb.NEUTRAL_BASE_LINEAR)
        r, g, b = px(self.res.base, right + 1, y + 5)
        self.assertNotEqual((r, g, b), (neutral, neutral, neutral))
        # an untouched pixel stays neutral
        self.assertEqual(px(self.res.base, 120, 120), (neutral, neutral, neutral))
        self.assertEqual(px(self.res.normal, 120, 120), (128, 128, 255))
        self.assertEqual(px(self.res.mre, 120, 120), (0, tb.clamp8(0.8), 0))

    # (2) numerals pairwise different per cell, each with dark pixels
    def test_panel_label_numerals(self):
        x, y, w, h = LABEL
        cw = w // 4
        cells = []
        for k in range(4):
            cell = []
            for yy in range(h):
                for xx in range(cw):
                    cell.append(px(self.res.base, x + k * cw + xx, y + yy)[0])
            cells.append(cell)
            dark = sum(1 for v in cell if v < 128)
            self.assertGreaterEqual(dark, 6, f"cell {k} has no dark numeral pixels")
            self.assertLess(dark, len(cell) // 2, f"cell {k} is mostly dark")
        for a in range(4):
            for b in range(a + 1, 4):
                self.assertNotEqual(cells[a], cells[b], f"cells {a} and {b} identical")
        # stroke width >= 2 px: every dark pixel has a dark horizontal or vertical neighbour
        cell = cells[0]
        for yy in range(h):
            for xx in range(cw):
                if cell[yy * cw + xx] < 128:
                    nb = []
                    if xx > 0:
                        nb.append(cell[yy * cw + xx - 1])
                    if xx < cw - 1:
                        nb.append(cell[yy * cw + xx + 1])
                    if yy > 0:
                        nb.append(cell[(yy - 1) * cw + xx])
                    if yy < h - 1:
                        nb.append(cell[(yy + 1) * cw + xx])
                    self.assertTrue(any(v < 128 for v in nb))

    # (3) state mask semantics
    def test_state_mask_collar_band_pulse(self):
        x, y, w, h = COLLAR
        r, g, b = px(self.res.state, x + w // 2, y + h // 2)
        self.assertAlmostEqual(r / 255.0, 5.0 / 8.0, delta=1.5 / 255.0)
        self.assertEqual(g, 0)
        self.assertEqual(b, 0)
        x, y, w, h = BAND
        self.assertEqual(px(self.res.state, x + 10, y + 2)[2], 255)
        self.assertEqual(px(self.res.state, x + 10, y + h + 8)[2], 0)
        self.assertEqual(px(self.res.state, 120, 120)[2], 0)
        x, y, w, h = PULSE
        gs = [px(self.res.state, x + i, y + h // 2)[1] for i in range(w)]
        for i in range(1, w):
            self.assertGreaterEqual(gs[i], gs[i - 1])
        self.assertGreater(gs[-1], gs[0])
        self.assertGreater(gs[-1], 200)
        self.assertLess(gs[0], 40)
        x, y, w, h = COUPLING
        self.assertEqual(px(self.res.state, x + 5, y + 2)[1], 255)

    # (4) emissive packing
    def test_mre_emissive(self):
        for rect in (COLLAR, PULSE, COUPLING):
            x, y, w, h = rect
            r, g, b = px(self.res.mre, x + w // 2, y + h // 2)
            self.assertEqual(b, 255)
            self.assertEqual(r, 0)
        for rect in (PLATE, BAND):
            x, y, w, h = rect
            r, g, b = px(self.res.mre, x + w // 2, y + h // 2)
            self.assertEqual(b, 0)
            self.assertEqual(r, tb.clamp8(0.04))
        x, y, w, h = WALL
        r, g, b = px(self.res.mre, x + w // 2, y + h // 2)
        self.assertGreater(r, 150)
        self.assertEqual(b, 0)

    # (5) normal map convention
    def test_normal_mean_flat(self):
        n = ATLAS * ATLAS
        buf = self.res.normal
        mean = [sum(buf[c::3]) / n for c in range(3)]
        self.assertAlmostEqual(mean[0], 128, delta=3)
        self.assertAlmostEqual(mean[1], 128, delta=3)
        self.assertGreater(mean[2], 245)

    def test_normal_directx_green_sign(self):
        # Height decreasing along +v -> the surface normal tilts toward +v_dir.
        def slope(job, px_, py_, lu_cm, lv_cm, P):
            return -0.05 * lv_cm

        res = tb.bake(make_manifest(shared_charts()[:1]), size=ATLAS, extra_height=slope)
        x, y, w, h = PLATE
        greens = [px(res.normal, x + w // 2, y + k)[1] for k in range(8, h - 8)]
        self.assertGreater(min(greens), 128)
        reds = [px(res.normal, x + w // 2, y + k)[0] for k in range(8, h - 8)]
        self.assertAlmostEqual(sum(reds) / len(reds), 128, delta=6)

    # (6) PNG round trip
    def test_png_dimensions(self):
        for name, buf in self.res.buffers().items():
            data = tb.encode_png_rgb(buf, ATLAS, ATLAS)
            w, h, pixels = tb.decode_png_rgb(data)
            self.assertEqual((w, h), (ATLAS, ATLAS), name)
            self.assertEqual(len(pixels), ATLAS * ATLAS * 3, name)
            self.assertEqual(bytes(buf), pixels, name)

    # (7) determinism
    def test_deterministic_bytes(self):
        again = tb.bake(make_manifest(shared_charts()), size=ATLAS)
        for name, buf in self.res.buffers().items():
            a = tb.encode_png_rgb(buf, ATLAS, ATLAS)
            b = tb.encode_png_rgb(again.buffers()[name], ATLAS, ATLAS)
            self.assertEqual(a, b, name)

    # panel plate markings and debug map sanity
    def test_panel_plate_grid_markings_and_debug(self):
        x, y, w, h = PLATE
        # the 3-cm border sits 6..9 cm from the plate edge = px 3..4.5 at 2 cm/px
        border = px(self.res.base, x + 3, y + h // 2)[0]
        interior = px(self.res.base, x + w // 2, y + h // 2)[0]
        self.assertLess(border, 128)
        self.assertGreater(interior, 200)
        # debug map: chart rect outlined in white, interior in slot colour
        self.assertEqual(px(self.res.debug, x, y), (255, 255, 255))
        self.assertEqual(px(self.res.debug, x + 2, y + 2)[:2], tb.DEBUG_SLOT_COLOURS["ceramic_civic"][:2])
        self.assertEqual(px(self.res.debug, 120, 120), (24, 24, 28))

    def test_rule_counts(self):
        rc = self.res.rule_counts
        self.assertEqual(rc.get("panel_plate_grid"), 1)
        self.assertEqual(rc.get("panel_label_numerals"), 1)
        self.assertEqual(rc.get("collar_segment"), 1)
        self.assertEqual(rc.get("conduit_pulse"), 1)
        self.assertEqual(rc.get("coupling_indicator"), 1)
        self.assertEqual(rc.get("team_band"), 1)
        self.assertEqual(rc.get("bottom_wear"), 1)
        self.assertEqual(self.res.unmatched, [])


class UnmatchedSlot(unittest.TestCase):
    def test_unknown_slot_reported_and_unpainted(self):
        chart = make_chart(9, "mystery", "MI_UNKNOWN", (4, 4, 10, 10), [0, 0, 0], [1, 0, 0], [0, 0, -1], [0, 1, 0])
        res = tb.bake(make_manifest([chart]), size=ATLAS)
        self.assertEqual(len(res.unmatched), 1)
        self.assertEqual(res.status[(9) * ATLAS + 9], 0)


class Glyphs(unittest.TestCase):
    def test_glyph_layout_fits_cell(self):
        for cw, ch in ((16, 9), (32, 18), (64, 36)):
            rects, gh, tw = tb.glyph_layout(cw, ch, "04")
            self.assertLessEqual(gh, ch)
            self.assertLessEqual(tw, cw)
            for x0, y0, x1, y1 in rects:
                self.assertGreaterEqual(x0, 0)
                self.assertGreaterEqual(y0, 0)
                self.assertLessEqual(x1, cw)
                self.assertLessEqual(y1, ch)
        # a cell too small for 2-px strokes is clipped, never an error
        self.assertEqual(len(tb.glyph_mask(8, 4, "04")), 32)
        masks = [tb.glyph_mask(16, 9, "%02d" % k) for k in range(1, 5)]
        for a in range(4):
            for b in range(a + 1, 4):
                self.assertNotEqual(masks[a], masks[b])




# --- Kharuun unit families (REL-ART-029; card REL-ART-005.KA.RIFTSTALKER) ------------------------
KHA_SLOTS = {"MI_EBS_KHA_Strata": "kharuun_obsidian", "MI_EBS_KHA_Amber": "kharuun_amber"}
SHELL = (4, 4, 48, 48)        # obsidian carapace plate, +X face
PLATE_MOLT = (60, 4, 30, 30)  # molt_plate_02: fresh growth
FOOT = (4, 60, 20, 20)        # rl_foot: ground wear
SEAM = (60, 60, 40, 6)        # seam_02_l: amber seam, long axis = u
CASTER = (60, 74, 40, 6)      # caster_slot: amber with heat gradient along u
PROW = (4, 90, 30, 16)        # prow: team carrier


def kharuun_charts():
    charts = [
        make_chart(0, "shell_02", "MI_EBS_KHA_Strata", SHELL, [40, 40, 120], [0, -1, 0], [0, 0, -1], [1, 0, 0]),
        make_chart(1, "molt_plate_02", "MI_EBS_KHA_Strata", PLATE_MOLT, [40, -40, 150], [0, -1, 0], [0, 0, -1], [1, 0, 0]),
        make_chart(2, "rl_foot", "MI_EBS_KHA_Strata", FOOT, [-90, 60, 20], [0, -1, 0], [0, 0, -1], [1, 0, 0]),
        make_chart(3, "seam_02_l", "MI_EBS_KHA_Amber", SEAM, [0, 40, 130], [1, 0, 0], [0, 0, -1], [0, 1, 0]),
        make_chart(4, "caster_slot", "MI_EBS_KHA_Amber", CASTER, [60, 0, 160], [1, 0, 0], [0, 0, -1], [0, 1, 0]),
        make_chart(5, "prow", "MI_EBS_KHA_Strata", PROW, [140, 20, 110], [0, -1, 0], [0, 0, -1], [1, 0, 0]),
    ]
    # authored COLOR_0 per component (R sweep order, G carapace, B striker, A reserved)
    colours = {"shell_02": (0.525, 0, 0, 1), "molt_plate_02": (1.0, 1.0, 0, 1), "rl_foot": (0, 0, 0, 1),
               "seam_02_l": (1.0, 0, 0, 1), "caster_slot": (0.15, 0, 0, 1), "prow": (0.85, 0, 0, 1)}
    for c in charts:
        c["vertex_color"] = list(colours[c["component"]])
    return charts


def kharuun_manifest(charts, blend=0):
    m = make_manifest(charts)
    m["slot_families"] = KHA_SLOTS
    m["team_components"] = ["prow"]
    if blend:
        m["molt_blend_size"] = blend
    return m


class KharuunBake(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = kharuun_manifest(kharuun_charts())
        cls.res = tb.bake(cls.manifest, size=ATLAS)

    def test_families_matched_and_rules_counted(self):
        self.assertEqual(self.res.unmatched, [])
        rc = self.res.rule_counts
        self.assertEqual(rc.get("family:kharuun_obsidian"), 4)
        self.assertEqual(rc.get("family:kharuun_amber"), 2)
        self.assertEqual(rc.get("kharuun_fresh_growth"), 1)
        self.assertEqual(rc.get("kharuun_foot_wear"), 1)
        self.assertEqual(rc.get("kharuun_seam"), 1)
        self.assertEqual(rc.get("kharuun_caster_heat"), 1)
        self.assertEqual(rc.get("team_carrier"), 1)

    def test_obsidian_is_a_charcoal_body_with_no_emissive(self):
        # Charcoal anchor: 0.02-0.07 linear. Sample the plate away from its gutters.
        x, y, w, h = SHELL
        def lin(c):
            c /= 255.0
            return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
        vals = []
        for yy in range(y + 6, y + h - 6, 2):
            for xx in range(x + 6, x + w - 6, 2):
                r, g, b = px(self.res.base, xx, yy)
                vals.append(0.2126 * lin(r) + 0.7152 * lin(g) + 0.0722 * lin(b))
                self.assertEqual(px(self.res.mre, xx, yy)[2], 0, "obsidian carries NO emissive")
                self.assertEqual(px(self.res.mre, xx, yy)[0], 0, "obsidian is non-metallic")
        # painted plate luminance must sit inside the Charcoal anchor (Docs/ArtDirection.md: 0.02-0.07 linear)
        self.assertLessEqual(max(vals), 0.075)
        self.assertGreaterEqual(min(vals), 0.018)
        self.assertLess(sum(vals) / len(vals), 0.06)

    def test_obsidian_is_warm_in_the_crack_bottoms_not_grey(self):
        # Somewhere on the plate the ember tint must push R above B (amber, not a neutral crack).
        x, y, w, h = SHELL
        warm = 0
        for yy in range(y + 4, y + h - 4):
            for xx in range(x + 4, x + w - 4):
                r, g, b = px(self.res.base, xx, yy)
                if r > b + 6:
                    warm += 1
        self.assertGreater(warm, 10)

    def test_fresh_growth_is_lighter_and_smoother_than_the_old_plate(self):
        def mean_over(rect, buf, ch):
            x, y, w, h = rect
            vals = [px(buf, xx, yy)[ch] for yy in range(y + 4, y + h - 4) for xx in range(x + 4, x + w - 4)]
            return sum(vals) / len(vals)
        self.assertGreater(mean_over(PLATE_MOLT, self.res.base, 0), mean_over(SHELL, self.res.base, 0))
        self.assertLess(mean_over(PLATE_MOLT, self.res.mre, 1), mean_over(SHELL, self.res.mre, 1))

    def test_amber_seam_is_the_only_emissive_and_is_brightest_on_its_centre_line(self):
        x, y, w, h = SEAM
        centre = px(self.res.mre, x + w // 2, y + h // 2)[2]
        edge = px(self.res.mre, x + w // 2, y + 1)[2]
        # a 6 px seam has no texel centred on its centre line (texel centres sit at +0.5), so the
        # brightest sample is one half-texel off the peak: >= 230, not 255
        self.assertGreaterEqual(centre, 230)
        self.assertGreater(centre, edge)
        r, g, b = px(self.res.base, x + w // 2, y + h // 2)
        self.assertGreater(r, g)
        self.assertGreater(g, b)
        # emissive accounting: seam + caster polygon texels only
        self.assertGreater(self.res.emissive_polygon_px, 0)
        n_amber = SEAM[2] * SEAM[3] + CASTER[2] * CASTER[3]
        self.assertLessEqual(self.res.emissive_polygon_px, n_amber + 4 * (SEAM[2] + SEAM[3] + CASTER[2] + CASTER[3]))

    def test_caster_heat_gradient_runs_along_the_slot(self):
        x, y, w, h = CASTER
        gs = [px(self.res.state, x + k, y + h // 2)[1] for k in range(2, w - 2, 6)]
        self.assertEqual(gs, sorted(gs))
        self.assertLess(gs[0], 40)
        self.assertGreater(gs[-1], 200)

    def test_state_mask_mirrors_color0_and_marks_team_and_core(self):
        # R mirrors COLOR_0.R exactly (within 8-bit) so texture and vertex data cannot disagree
        for rect, comp, r_expect in ((SHELL, "shell_02", 0.525), (PROW, "prow", 0.85), (FOOT, "rl_foot", 0.0)):
            x, y, w, h = rect
            self.assertAlmostEqual(px(self.res.state, x + w // 2, y + h // 2)[0] / 255.0, r_expect, delta=0.004, msg=comp)
        # team carrier only on the prow
        x, y, w, h = PROW
        self.assertEqual(px(self.res.state, x + w // 2, y + h // 2)[2], 255)
        x, y, w, h = SHELL
        self.assertEqual(px(self.res.state, x + w // 2, y + h // 2)[2], 0)
        # core blend only on new growth, peaking at the chart centre
        x, y, w, h = PLATE_MOLT
        self.assertGreater(px(self.res.state, x + w // 2, y + h // 2)[1], 230)
        self.assertLess(px(self.res.state, x + 3, y + 3)[1], 80)
        x, y, w, h = SHELL
        self.assertEqual(px(self.res.state, x + w // 2, y + h // 2)[1], 0)

    def test_foot_wear_dusts_the_ground_contact(self):
        x, y, w, h = FOOT
        low = px(self.res.mre, x + w // 2, y + h - 5)[1]   # near the ground (v down)
        high = px(self.res.mre, x + w // 2, y + 5)[1]
        self.assertGreaterEqual(low, high)

    def test_debug_colours_and_normal_convention_hold(self):
        x, y, w, h = SHELL
        self.assertEqual(px(self.res.debug, x + 2, y + 2)[:2], tb.DEBUG_SLOT_COLOURS["kharuun_obsidian"][:2])
        n = ATLAS * ATLAS
        mean = [sum(self.res.normal[c::3]) / n for c in range(3)]
        self.assertAlmostEqual(mean[0], 128, delta=3)
        self.assertAlmostEqual(mean[1], 128, delta=3)

    def test_deterministic_and_molt_blend_map(self):
        import tempfile
        again = tb.bake(kharuun_manifest(kharuun_charts()), size=ATLAS)
        for name, buf in self.res.buffers().items():
            self.assertEqual(bytes(buf), bytes(again.buffers()[name]), name)
        with tempfile.TemporaryDirectory() as tmp:
            m = kharuun_manifest(kharuun_charts(), blend=32)
            res = tb.bake(m, size=ATLAS)
            report = tb.write_outputs(res, tmp, "EBS-TEST-000", None, None, m, 0.0)
            self.assertIn("moltblend", report["maps"])
            self.assertEqual(report["maps"]["moltblend"]["size"], 32)
            self.assertTrue(os.path.exists(os.path.join(tmp, "T_EBS_TEST_000_MoltBlend.png")))
            self.assertGreater(report["emissive"]["fraction_of_painted_polygon_area"], 0.0)
            self.assertLess(report["emissive"]["fraction_of_painted_polygon_area"], 0.5)

if __name__ == "__main__":
    unittest.main()
