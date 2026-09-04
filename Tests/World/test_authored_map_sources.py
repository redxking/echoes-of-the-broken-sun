#!/usr/bin/env python3
"""Assurance tests for all authored overlay map source contracts.

Author: Angelis Pseftis
Zero runtime dependency (Python standard library only).
Validates that every authored map source in Content/World/Source/:
1. Has standard 64x64 grid layout with southwest origin.
2. Evaluates rectangle operations (block/open) to the exact declared census.
3. Leaves all declared shared sites unblocked (unless explicitly recorded).
4. Maintains single-component passable graph connectivity (no islands).
5. Carries an authoritative claim boundary and author attribution.
"""

from __future__ import annotations

import json
import unittest
from collections import deque
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
SOURCE_DIR = REPO_ROOT / "Content/World/Source"


class TestAuthoredMapSources(unittest.TestCase):
    def setUp(self) -> None:
        self.sources = sorted(SOURCE_DIR.rglob("*map_source*.json"))
        self.assertGreater(
            len(self.sources), 0, "No map source files found under Content/World/Source"
        )

    def test_overlay_map_sources_evaluate_deterministically(self) -> None:
        overlay_sources_count = 0
        for path in self.sources:
            content = path.read_text(encoding="utf-8")
            data = json.loads(content)
            if data.get("source_format") != "echoes-overlay-map-source":
                continue

            overlay_sources_count += 1
            rel_path = path.relative_to(REPO_ROOT)
            with self.subTest(file=str(rel_path)):
                grid = data["grid"]
                self.assertEqual(grid["width_tiles"], 64)
                self.assertEqual(grid["height_tiles"], 64)
                self.assertEqual(grid["coordinate_origin"], "southwest")

                # Author attribution verification on newly authored maps
                if "campaign" in path.name or "shivergrass" in path.name or "parity" in path.name or "sector9" in path.name or "dais" in path.name or "unburied" in path.name:
                    authority_note = data.get("authority_note", "")
                    self.assertIn("Angelis Pseftis", authority_note)

                # Claim boundary verification
                claim = data.get("claim_boundary", {})
                self.assertTrue(claim.get("proposed_contract", False))

                shared_sites = {
                    site["id"]: (site["x"], site["y"])
                    for site in data.get("shared_sites", [])
                }

                for variant in data.get("variants", []):
                    variant_id = variant["id"]
                    blocked: set[int] = set()
                    for op in variant["ops"]:
                        x0, x1 = op["x0"], op["x1"]
                        y0, y1 = op["y0"], op["y1"]
                        self.assertTrue(0 <= x0 <= x1 < 64)
                        self.assertTrue(0 <= y0 <= y1 < 64)

                        if op["op"] == "block":
                            for y in range(y0, y1 + 1):
                                for x in range(x0, x1 + 1):
                                    blocked.add(y * 64 + x)
                        elif op["op"] == "open":
                            for y in range(y0, y1 + 1):
                                for x in range(x0, x1 + 1):
                                    blocked.discard(y * 64 + x)

                    # 1. Exact census match
                    expected_cells = variant["expected_blocked_cells"]
                    self.assertEqual(
                        len(blocked),
                        expected_cells,
                        f"Census mismatch in {variant_id} of {rel_path}: got {len(blocked)}, expected {expected_cells}",
                    )

                    # 2. Blocked sites agreement
                    blocked_sites = [
                        sid
                        for sid, (sx, sy) in shared_sites.items()
                        if (sy * 64 + sx) in blocked
                    ]
                    expected_blocked_sites = variant.get("expected_blocked_sites", [])
                    self.assertEqual(
                        set(blocked_sites),
                        set(expected_blocked_sites),
                        f"Blocked sites mismatch in {variant_id} of {rel_path}",
                    )

                    # 3. Passable graph connectivity
                    all_open = set(range(4096)) - blocked
                    visited: set[int] = set()
                    components = 0
                    for cell in all_open:
                        if cell not in visited:
                            components += 1
                            queue = deque([cell])
                            visited.add(cell)
                            while queue:
                                c = queue.popleft()
                                cx, cy = c % 64, c // 64
                                for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                                    nx, ny = cx + dx, cy + dy
                                    if 0 <= nx < 64 and 0 <= ny < 64:
                                        nc = ny * 64 + nx
                                        if nc in all_open and nc not in visited:
                                            visited.add(nc)
                                            queue.append(nc)

                    expected_components = variant.get("expected_passable_components", 1)
                    self.assertEqual(
                        components,
                        expected_components,
                        f"Component count mismatch in {variant_id} of {rel_path}",
                    )

        self.assertGreaterEqual(overlay_sources_count, 5)


if __name__ == "__main__":
    unittest.main()
