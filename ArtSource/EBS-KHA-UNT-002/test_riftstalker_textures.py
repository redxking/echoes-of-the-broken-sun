#!/usr/bin/env python3
"""Texture-stage checks for EBS-KHA-UNT-002 (BLOCKOUT bake against a PROVISIONAL reading of the card).

Author: Angelis Pseftis. Run: python3 test_riftstalker_textures.py
Needs the evidence root's textures/bake-report.json; skips otherwise.
"""
from __future__ import annotations

import json
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_texbake as tb  # noqa: E402
import build_riftstalker as rs  # noqa: E402

EVR = os.environ.get("EBS_EVIDENCE_ROOT",
                     "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z")
TEX = os.environ.get("EBS_TEX_DIR", os.path.join(EVR, "EBS-KHA-UNT-002", "textures"))
REPORT = os.path.join(TEX, "bake-report.json")
MANIFEST = os.path.join(HERE, "bake-manifest.json")


def load(path):
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


class RiftstalkerBake(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (os.path.exists(REPORT) and os.path.exists(MANIFEST)):
            raise unittest.SkipTest("no bake report / manifest")
        cls.report = load(REPORT)
        cls.manifest = load(MANIFEST)

    def test_report_matches_this_manifest_and_revision(self):
        self.assertEqual(self.report["manifest_sha256"], tb.sha256_file(MANIFEST))
        self.assertEqual(self.report["manifest_revision"], rs.REVISION)
        self.assertEqual(self.report["size"], rs.ATLAS_SIZE)
        self.assertEqual(self.report["charts_total"], len(self.manifest["atlas"]["charts"]))

    def test_every_chart_painted_none_unmatched_or_skipped(self):
        self.assertEqual(self.report["charts_unmatched"], [])
        self.assertEqual(self.report["charts_skipped"], [])
        rc = self.report["rule_counts"]
        self.assertGreater(rc.get("family:kharuun_obsidian", 0), 0)
        self.assertGreater(rc.get("family:kharuun_amber", 0), 0)
        self.assertGreater(rc.get("kharuun_fresh_growth", 0), 0)
        self.assertGreater(rc.get("team_carrier", 0), 0)

    def test_the_full_stack_exists_with_recorded_hashes(self):
        maps = self.report["maps"]
        for key, suffix in (("basecolor", "BaseColor"), ("normal", "Normal"), ("mre", "MRE"),
                            ("statemask", "StateMask"), ("debug", "AtlasDebug"), ("moltblend", "MoltBlend")):
            self.assertIn(key, maps, key)
            path = os.path.join(TEX, maps[key]["file"])
            self.assertTrue(os.path.exists(path), path)
            self.assertEqual(tb.sha256_file(path), maps[key]["sha256"], key)
            self.assertTrue(maps[key]["file"].endswith(f"_{suffix}.png"))
        self.assertEqual(maps["moltblend"]["size"], rs.MOLT_BLEND_SIZE)

    def test_amber_emissive_is_under_the_ceiling_by_baked_area(self):
        # the card's <=15% is a CEILING measured here on the baked MRE.B mask by painted polygon area
        frac = self.report["emissive"]["fraction_of_painted_polygon_area"]
        self.assertGreater(frac, 0.0, "the seams must be lit")
        self.assertLessEqual(frac, 0.15)
        # and it agrees with the slot geometry within a gutter's worth
        self.assertAlmostEqual(frac, rs.slot_area_fraction(rs.assemble(0, "baseline")[0], rs.AMBER), delta=0.03)

    def test_emissive_lives_only_on_amber_charts(self):
        # sample the MRE map at each chart centre: obsidian charts carry B == 0, amber charts B > 0
        w, h, pixels = tb.decode_png_rgb(open(os.path.join(TEX, self.report["maps"]["mre"]["file"]), "rb").read())
        size = self.manifest["atlas"]["size"]
        fam = self.manifest["slot_families"]
        bad = []
        for chart in self.manifest["atlas"]["charts"]:
            uv = chart["polygon_uv"]
            cx = int(sum(p[0] for p in uv) / len(uv) * size)
            cy = int(sum(p[1] for p in uv) / len(uv) * size)
            b = pixels[(cy * w + cx) * 3 + 2]
            family = fam[chart["slot"]]
            if family == "kharuun_obsidian" and b != 0:
                bad.append((chart["component"], "obsidian lit", b))
            if family == "kharuun_amber" and b == 0:
                bad.append((chart["component"], "amber dark", b))
        self.assertEqual(bad, [])

    def test_state_mask_mirrors_color0_at_chart_centres(self):
        w, h, pixels = tb.decode_png_rgb(open(os.path.join(TEX, self.report["maps"]["statemask"]["file"]), "rb").read())
        size = self.manifest["atlas"]["size"]
        worst = 0.0
        team = set(self.manifest["team_components"])
        for chart in self.manifest["atlas"]["charts"]:
            uv = chart["polygon_uv"]
            cx = int(sum(p[0] for p in uv) / len(uv) * size)
            cy = int(sum(p[1] for p in uv) / len(uv) * size)
            r, g, b = pixels[(cy * w + cx) * 3:(cy * w + cx) * 3 + 3]
            worst = max(worst, abs(r / 255.0 - chart["vertex_color"][0]))
            self.assertEqual(b == 255, chart["component"] in team, chart["component"])
        self.assertLess(worst, 0.006, "StateMask.R must mirror COLOR_0.R within 8-bit")

    def test_bake_is_deterministic(self):
        a = tb.bake(self.manifest, size=256)
        b = tb.bake(self.manifest, size=256)
        for name, buf in a.buffers().items():
            self.assertEqual(bytes(buf), bytes(b.buffers()[name]), name)


if __name__ == "__main__":
    unittest.main(verbosity=1)
