#!/usr/bin/env python3
"""Compile the overlay/preset map pack from the three authoring sources.

Reads the Glass Scar overlay source and the two skirmish preset sources,
applies each variant's rectangle operations onto its declared base (the
digest-gated compiled Glass Scar v2 fixture, or an open grid), verifies every
declared expectation (blocked census, passable connectivity, blocked-site
list), and emits the canonical compiled fixture plus its digest sidecar.

Fail-closed: any structural, digest, census, connectivity, or expectation
mismatch refuses compilation. The tool writes only the two paths under
``Content/World/Generated/Overlays/`` and never touches the authoring
sources, the accepted v1/v2 fixtures, or the accepted compiler
``compile_map_pack.py`` (which stays byte-frozen).

Determinism: the emitted pack is a pure function of the four input files;
two runs are byte-identical.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import deque
from pathlib import Path
from typing import Any

GRID_WIDTH = 64
GRID_HEIGHT = 64
CELL_COUNT = GRID_WIDTH * GRID_HEIGHT

BASE_PACK_RELATIVE_PATH = Path(
    "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
)
SOURCE_RELATIVE_PATHS = (
    Path("Content/World/Source/GlassScar/glass_scar_overlays_v1.json"),
    Path("Content/World/Source/Skirmish/crownfall_basin_map_source_v1.json"),
    Path("Content/World/Source/Skirmish/soryn_confluence_map_source_v1.json"),
)
OUTPUT_PACK_RELATIVE_PATH = Path(
    "Content/World/Generated/Overlays/overlay_map_packs_v1.json"
)
OUTPUT_SIDECAR_RELATIVE_PATH = Path(
    "Content/World/Generated/Overlays/overlay_map_packs_v1.sha256"
)

EXPECTED_BASE_PACK_SHA256 = (
    "6dbbe2f1a5bd7faec122e6050e5c2d27d4d56e3e94da357b239673ec4b6ab621"
)
RUNTIME_MIRROR_COMMIT = "6fa2d6f1355687da990b57fca6c620c3e18866c1"


class CompileError(RuntimeError):
    """Raised for any condition that must fail closed."""


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise CompileError(f"duplicate JSON object key: {key!r}")
        result[key] = value
    return result


def reject_nonfinite(constant: str) -> None:
    raise CompileError(f"non-finite JSON constant rejected: {constant}")


def load_strict_json(path: Path) -> tuple[dict[str, Any], str]:
    if not path.is_file():
        raise CompileError(f"input missing: {path}")
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    value = json.loads(
        raw.decode("utf-8"),
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )
    if not isinstance(value, dict):
        raise CompileError(f"{path}: root is not a JSON object")
    return value, digest


def require_int(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise CompileError(f"{label} must be a JSON integer, got {value!r}")
    return value


def load_base_blocked(repo_root: Path) -> frozenset[int]:
    path = repo_root / BASE_PACK_RELATIVE_PATH
    if not path.is_file():
        raise CompileError(f"base compiled pack missing: {path}")
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED_BASE_PACK_SHA256:
        raise CompileError(
            "base compiled pack digest mismatch: "
            f"computed {digest}, contract pins {EXPECTED_BASE_PACK_SHA256}"
        )
    pack = json.loads(
        raw.decode("utf-8"),
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )
    mask = pack["cells"]["movement_mask"]
    if not isinstance(mask, list) or len(mask) != CELL_COUNT:
        raise CompileError("base pack movement mask is not a 4096-entry array")
    return frozenset(
        index for index, value in enumerate(mask) if (value & 1) == 0
    )


def apply_ops(base_blocked: frozenset[int], ops: Any, label: str) -> set[int]:
    if not isinstance(ops, list):
        raise CompileError(f"{label}: ops must be an array")
    blocked = set(base_blocked)
    for position, op in enumerate(ops):
        op_label = f"{label}.ops[{position}]"
        if not isinstance(op, dict):
            raise CompileError(f"{op_label} is not an object")
        kind = op.get("op")
        if kind not in ("block", "open"):
            raise CompileError(f"{op_label}: unknown op {kind!r}")
        x0 = require_int(op.get("x0"), f"{op_label}.x0")
        x1 = require_int(op.get("x1"), f"{op_label}.x1")
        y0 = require_int(op.get("y0"), f"{op_label}.y0")
        y1 = require_int(op.get("y1"), f"{op_label}.y1")
        if not (0 <= x0 <= x1 < GRID_WIDTH and 0 <= y0 <= y1 < GRID_HEIGHT):
            raise CompileError(f"{op_label}: rectangle out of range or inverted")
        unknown = set(op) - {"op", "x0", "x1", "y0", "y1"}
        if unknown:
            raise CompileError(f"{op_label}: unknown fields {sorted(unknown)}")
        for y in range(y0, y1 + 1):
            row = y * GRID_WIDTH
            for x in range(x0, x1 + 1):
                if kind == "block":
                    blocked.add(row + x)
                else:
                    blocked.discard(row + x)
    return blocked


def count_components(blocked: set[int]) -> int:
    seen = [False] * CELL_COUNT
    components = 0
    for start in range(CELL_COUNT):
        if seen[start] or start in blocked:
            continue
        components += 1
        queue = deque((start,))
        seen[start] = True
        while queue:
            index = queue.popleft()
            x = index % GRID_WIDTH
            y = index // GRID_WIDTH
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < GRID_WIDTH and 0 <= ny < GRID_HEIGHT:
                    neighbor = ny * GRID_WIDTH + nx
                    if not seen[neighbor] and neighbor not in blocked:
                        seen[neighbor] = True
                        queue.append(neighbor)
    return components


def reachable_from(blocked: set[int], start: int) -> set[int]:
    if start in blocked:
        return set()
    seen = {start}
    queue = deque((start,))
    while queue:
        index = queue.popleft()
        x = index % GRID_WIDTH
        y = index // GRID_WIDTH
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < GRID_WIDTH and 0 <= ny < GRID_HEIGHT:
                neighbor = ny * GRID_WIDTH + nx
                if neighbor not in blocked and neighbor not in seen:
                    seen.add(neighbor)
                    queue.append(neighbor)
    return seen


def compile_variants(
    source: dict[str, Any],
    source_label: str,
    base_blocked: frozenset[int],
) -> list[dict[str, Any]]:
    if source.get("source_format") != "echoes-overlay-map-source":
        raise CompileError(f"{source_label}: unexpected source_format")
    if source.get("source_version") != 1:
        raise CompileError(f"{source_label}: unexpected source_version")
    grid = source.get("grid")
    if (
        not isinstance(grid, dict)
        or grid.get("width_tiles") != GRID_WIDTH
        or grid.get("height_tiles") != GRID_HEIGHT
        or grid.get("index_formula") != "y*width+x"
        or grid.get("coordinate_origin") != "southwest"
    ):
        raise CompileError(f"{source_label}: grid contract drifted")
    shared_sites = source.get("shared_sites")
    if not isinstance(shared_sites, list) or not shared_sites:
        raise CompileError(f"{source_label}: shared_sites missing")
    site_ids = [site.get("id") for site in shared_sites]
    if len(site_ids) != len(set(site_ids)):
        raise CompileError(f"{source_label}: duplicate site ids")
    variants = source.get("variants")
    if not isinstance(variants, list) or not variants:
        raise CompileError(f"{source_label}: variants missing")
    compiled: list[dict[str, Any]] = []
    for variant in variants:
        variant_id = variant.get("id")
        label = f"{source_label}:{variant_id}"
        base_name = variant.get("base")
        if base_name == "glass-scar-v1":
            base = base_blocked
        elif base_name == "open-grid":
            base = frozenset()
        else:
            raise CompileError(f"{label}: unknown base {base_name!r}")
        blocked = apply_ops(base, variant.get("ops"), label)
        expected_blocked = require_int(
            variant.get("expected_blocked_cells"), f"{label}.expected_blocked_cells"
        )
        if len(blocked) != expected_blocked:
            raise CompileError(
                f"{label}: blocked census {len(blocked)} diverges from the "
                f"declared expectation {expected_blocked}"
            )
        components = count_components(blocked)
        expected_components = require_int(
            variant.get("expected_passable_components"),
            f"{label}.expected_passable_components",
        )
        if components != expected_components:
            raise CompileError(
                f"{label}: passable component count {components} diverges from "
                f"the declared expectation {expected_components}"
            )
        expected_blocked_sites = variant.get("expected_blocked_sites")
        if not isinstance(expected_blocked_sites, list):
            raise CompileError(f"{label}: expected_blocked_sites missing")
        unknown_expected = set(expected_blocked_sites) - set(site_ids)
        if unknown_expected:
            raise CompileError(
                f"{label}: expected_blocked_sites names unknown sites "
                f"{sorted(unknown_expected)}"
            )
        first_passable: int | None = None
        site_records: list[dict[str, Any]] = []
        site_indices: dict[str, int] = {}
        for site in shared_sites:
            x = require_int(site.get("x"), f"{label}.site.x")
            y = require_int(site.get("y"), f"{label}.site.y")
            if not (0 <= x < GRID_WIDTH and 0 <= y < GRID_HEIGHT):
                raise CompileError(f"{label}: site {site.get('id')!r} out of range")
            index = y * GRID_WIDTH + x
            site_indices[site["id"]] = index
            if index not in blocked and first_passable is None:
                first_passable = index
        reach = (
            reachable_from(blocked, first_passable)
            if first_passable is not None
            else set()
        )
        actual_blocked_sites: list[str] = []
        for site in shared_sites:
            index = site_indices[site["id"]]
            passable = index not in blocked
            if not passable:
                actual_blocked_sites.append(site["id"])
            site_records.append(
                {
                    "id": site["id"],
                    "x": index % GRID_WIDTH,
                    "y": index // GRID_WIDTH,
                    "passable": passable,
                    "reachable": index in reach,
                }
            )
        if sorted(actual_blocked_sites) != sorted(expected_blocked_sites):
            raise CompileError(
                f"{label}: blocked sites {sorted(actual_blocked_sites)} diverge "
                f"from the declared expectation {sorted(expected_blocked_sites)}"
            )
        for record in site_records:
            if record["passable"] and not record["reachable"]:
                raise CompileError(
                    f"{label}: passable site {record['id']!r} is unreachable "
                    "from the first passable site"
                )
        compiled.append(
            {
                "id": variant_id,
                "family": variant.get("family"),
                "branch": variant.get("branch"),
                "base": base_name,
                "blocked_cell_count": len(blocked),
                "passable_cell_count": CELL_COUNT - len(blocked),
                "passable_component_count": components,
                "blocked_cell_indices": sorted(blocked),
                "sites": site_records,
            }
        )
    return compiled


def build_pack(repo_root: Path) -> dict[str, Any]:
    base_blocked = load_base_blocked(repo_root)
    variants: list[dict[str, Any]] = []
    provenance: list[dict[str, str]] = []
    for relative in SOURCE_RELATIVE_PATHS:
        source, digest = load_strict_json(repo_root / relative)
        provenance.append({"path": relative.as_posix(), "raw_sha256": digest})
        variants.extend(compile_variants(source, relative.name, base_blocked))
    ids = [variant["id"] for variant in variants]
    if len(ids) != len(set(ids)):
        raise CompileError("duplicate variant ids across sources")
    variants.sort(key=lambda variant: variant["id"])
    return {
        "pack_format": "echoes-overlay-map-pack",
        "pack_version": 1,
        "schema_version": 1,
        "authority": "checked-in-source-fixture",
        "grid": {
            "width_tiles": GRID_WIDTH,
            "height_tiles": GRID_HEIGHT,
            "cell_count": CELL_COUNT,
            "index_formula": "y*width+x",
            "coordinate_origin": "southwest",
        },
        "base_contract": {
            "compiled_pack_path": BASE_PACK_RELATIVE_PATH.as_posix(),
            "compiled_pack_sha256": EXPECTED_BASE_PACK_SHA256,
        },
        "source_provenance": {
            "runtime_mirror_commit": RUNTIME_MIRROR_COMMIT,
            "sources": provenance,
        },
        "variants": variants,
        "claim_boundary": {
            "proposed_contract": True,
            "compiled_fixture_is_source_data": True,
            "not_evidence_for": [
                "runtime consumption of this data",
                "mission plan sites or objective placement",
                "spawn balance or resource pacing",
                "measured travel time or gameplay pacing",
                "NavMesh, formation traversal, or camera behavior",
                "package, performance, or release readiness",
            ],
        },
    }


def render_bytes(pack: dict[str, Any]) -> bytes:
    return json.dumps(
        pack, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("utf-8")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Project root containing Content/ (default: current directory).",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--check",
        action="store_true",
        help="Verify the checked-in fixture and sidecar match (default mode).",
    )
    mode.add_argument(
        "--write",
        action="store_true",
        help="Rewrite the compiled fixture and sidecar.",
    )
    args = parser.parse_args(argv)
    repo_root = args.repo_root.resolve()
    try:
        pack = build_pack(repo_root)
    except CompileError as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 1
    rendered = render_bytes(pack)
    digest = hashlib.sha256(rendered).hexdigest()
    pack_path = repo_root / OUTPUT_PACK_RELATIVE_PATH
    sidecar_path = repo_root / OUTPUT_SIDECAR_RELATIVE_PATH
    if args.write:
        pack_path.parent.mkdir(parents=True, exist_ok=True)
        pack_path.write_bytes(rendered)
        sidecar_path.write_text(digest + "\n", encoding="utf-8", newline="\n")
        print(
            f"wrote {pack_path} ({len(rendered)} bytes) and sidecar; "
            f"digest={digest}"
        )
        return 0
    if not pack_path.is_file() or not sidecar_path.is_file():
        print(
            "REFUSED: checked-in overlay fixture or sidecar missing; "
            "compile with --write and re-review",
            file=sys.stderr,
        )
        return 1
    existing = pack_path.read_bytes()
    existing_sidecar = sidecar_path.read_text(encoding="utf-8").strip()
    if existing != rendered or existing_sidecar != digest:
        print(
            "REFUSED: checked-in overlay fixture or sidecar does not match "
            "the sources; recompile with --write and re-review",
            file=sys.stderr,
        )
        return 1
    print(f"overlay pack matches sources; digest={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
