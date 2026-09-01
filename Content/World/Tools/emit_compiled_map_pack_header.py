#!/usr/bin/env python3
"""Emit the checked-in C++ constant-table header for the Glass Scar compiled map pack.

Reads the frozen compiled fixture and its digest sidecar, fails closed on any
identity or structural mismatch, and deterministically renders
``Source/EchoesOfTheBrokenSun/Public/EchoesGlassScarCompiledMapPack.h``.

The emitted header is a pure function of the compiled pack bytes: two runs over
the same fixture are byte-identical. The tool never touches the compiled pack,
its sidecar, or any other repository file; ``--write`` may replace only the one
generated header path, and ``--check`` (the default) only compares.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

PACK_RELATIVE_PATH = Path(
    "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
)
SIDECAR_RELATIVE_PATH = Path(
    "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.sha256"
)
HEADER_RELATIVE_PATH = Path(
    "Source/EchoesOfTheBrokenSun/Public/EchoesGlassScarCompiledMapPack.h"
)

EXPECTED_PACK_FORMAT = "echoes-compiled-map-pack"
EXPECTED_MAP_ID = "glass-scar"
EXPECTED_PACK_VERSION = 1
EXPECTED_SCHEMA_VERSION = 1
EXPECTED_CELL_COUNT = 4096
EXPECTED_BLOCKED_CELLS = 165
EXPECTED_PASSABLE_CELLS = 3931
VALUES_PER_LINE = 32


class EmitError(RuntimeError):
    """Raised for any condition that must fail closed."""


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise EmitError(f"duplicate JSON object key: {key!r}")
        result[key] = value
    return result


def reject_nonfinite(constant: str) -> None:
    raise EmitError(f"non-finite JSON constant rejected: {constant}")


def load_pack(repo_root: Path) -> tuple[dict[str, Any], str]:
    pack_path = repo_root / PACK_RELATIVE_PATH
    sidecar_path = repo_root / SIDECAR_RELATIVE_PATH
    if not pack_path.is_file():
        raise EmitError(f"compiled pack missing: {pack_path}")
    if not sidecar_path.is_file():
        raise EmitError(f"digest sidecar missing: {sidecar_path}")
    raw = pack_path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    sidecar = sidecar_path.read_text(encoding="utf-8").strip()
    if len(sidecar) != 64 or any(c not in "0123456789abcdef" for c in sidecar):
        raise EmitError(f"sidecar is not a bare lowercase SHA-256 hex digest: {sidecar!r}")
    if digest != sidecar:
        raise EmitError(
            "compiled pack digest mismatch: "
            f"computed {digest} but sidecar records {sidecar}"
        )
    pack = json.loads(
        raw.decode("utf-8"),
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )
    if not isinstance(pack, dict):
        raise EmitError("compiled pack root is not a JSON object")
    return pack, digest


def require_int(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise EmitError(f"{label} must be a JSON integer, got {value!r}")
    return value


def require_str(value: Any, label: str) -> str:
    if not isinstance(value, str):
        raise EmitError(f"{label} must be a JSON string, got {value!r}")
    return value


def require_uint8_array(values: Any, label: str) -> list[int]:
    if not isinstance(values, list) or len(values) != EXPECTED_CELL_COUNT:
        raise EmitError(f"{label} must be a {EXPECTED_CELL_COUNT}-entry array")
    out: list[int] = []
    for index, value in enumerate(values):
        entry = require_int(value, f"{label}[{index}]")
        if entry < 0 or entry > 255:
            raise EmitError(f"{label}[{index}] out of uint8 range: {entry}")
        out.append(entry)
    return out


def validate(pack: dict[str, Any]) -> None:
    if require_str(pack.get("pack_format"), "pack_format") != EXPECTED_PACK_FORMAT:
        raise EmitError("unexpected pack_format")
    if require_str(pack.get("map_id"), "map_id") != EXPECTED_MAP_ID:
        raise EmitError("unexpected map_id")
    if require_int(pack.get("pack_version"), "pack_version") != EXPECTED_PACK_VERSION:
        raise EmitError("unexpected pack_version")
    if require_int(pack.get("schema_version"), "schema_version") != EXPECTED_SCHEMA_VERSION:
        raise EmitError("unexpected schema_version")
    grid = pack["grid"]
    if (
        require_int(grid.get("width_tiles"), "grid.width_tiles") != 64
        or require_int(grid.get("height_tiles"), "grid.height_tiles") != 64
        or require_int(grid.get("cell_count"), "grid.cell_count") != EXPECTED_CELL_COUNT
        or require_str(grid.get("index_formula"), "grid.index_formula") != "y*width+x"
    ):
        raise EmitError("grid contract drifted from the 64x64 row-major layout")
    cells = pack["cells"]
    mask = require_uint8_array(cells.get("movement_mask"), "cells.movement_mask")
    cost = require_uint8_array(cells.get("base_move_cost"), "cells.base_move_cost")
    blocked = sum(1 for value in mask if (value & 1) == 0)
    passable = EXPECTED_CELL_COUNT - blocked
    if blocked != EXPECTED_BLOCKED_CELLS or passable != EXPECTED_PASSABLE_CELLS:
        raise EmitError(
            f"blocked/passable census drifted: {blocked}/{passable} versus the "
            f"contract {EXPECTED_BLOCKED_CELLS}/{EXPECTED_PASSABLE_CELLS}"
        )
    for index in range(EXPECTED_CELL_COUNT):
        ground = (mask[index] & 1) != 0
        if ground and cost[index] == 0:
            raise EmitError(f"passable cell {index} carries blocked cost 0")
        if not ground and cost[index] != 0:
            raise EmitError(f"blocked cell {index} carries nonzero cost {cost[index]}")


def format_array(name: str, values: list[int]) -> list[str]:
    lines = [
        f"inline constexpr std::uint8_t {name}[kCellCount] = {{",
    ]
    for start in range(0, len(values), VALUES_PER_LINE):
        chunk = values[start : start + VALUES_PER_LINE]
        lines.append("    " + ",".join(str(value) for value in chunk) + ",")
    lines.append("};")
    return lines


def cpp_string(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def render_header(pack: dict[str, Any], digest: str) -> str:
    validate(pack)
    grid = pack["grid"]
    cells = pack["cells"]
    source = pack["source_contract"]
    camera = pack["camera_bounds"]
    mask = require_uint8_array(cells["movement_mask"], "cells.movement_mask")
    cost = require_uint8_array(cells["base_move_cost"], "cells.base_move_cost")
    height = require_uint8_array(cells["height_band_ordinal"], "cells.height_band_ordinal")
    region = require_uint8_array(cells["region_ordinal"], "cells.region_ordinal")

    lines: list[str] = []
    push = lines.append
    push("// EchoesGlassScarCompiledMapPack.h")
    push("// GENERATED FILE - do not edit by hand.")
    push("// Generated by Content/World/Tools/emit_compiled_map_pack_header.py from the")
    push(f"// frozen compiled fixture {PACK_RELATIVE_PATH.as_posix()}.")
    push("// Verify:   python3 Content/World/Tools/emit_compiled_map_pack_header.py --check")
    push("// Rewrite:  python3 Content/World/Tools/emit_compiled_map_pack_header.py --write")
    push("// The tables below are checked-in source contract data, not runtime output;")
    push("// runtime_binding of the underlying pack remains as recorded in the fixture.")
    push("")
    push("#pragma once")
    push("")
    push("#include <cstdint>")
    push("")
    push("namespace echoes::world::glass_scar_pack")
    push("{")
    push("")
    push(f"inline constexpr std::int32_t kGridWidthTiles = {grid['width_tiles']};")
    push(f"inline constexpr std::int32_t kGridHeightTiles = {grid['height_tiles']};")
    push("// index = y * kGridWidthTiles + x, row-major from the southwest origin.")
    push(f"inline constexpr std::int32_t kCellCount = {grid['cell_count']};")
    push(
        "inline constexpr std::int32_t kSimulationTileCentimeters = "
        f"{require_int(grid['simulation_tile_centimeters'], 'grid.simulation_tile_centimeters')};"
    )
    push(
        "inline constexpr std::int32_t kPresentationTileWorldUnits = "
        f"{require_int(grid['presentation_tile_world_units'], 'grid.presentation_tile_world_units')};"
    )
    push("")
    push(f"inline constexpr char kPackFormat[] = {cpp_string(pack['pack_format'])};")
    push(f"inline constexpr char kMapId[] = {cpp_string(pack['map_id'])};")
    push(f"inline constexpr std::int32_t kPackVersion = {pack['pack_version']};")
    push(f"inline constexpr std::int32_t kSchemaVersion = {pack['schema_version']};")
    push(f"inline constexpr char kCompiledPackSha256[] = {cpp_string(digest)};")
    push(
        "inline constexpr char kAuthoringCanonicalSha256[] = "
        f"{cpp_string(require_str(source['authoring_canonical_sha256'], 'source.authoring_canonical_sha256'))};"
    )
    push(
        "inline constexpr char kBaseDescriptorCanonicalSha256[] = "
        f"{cpp_string(require_str(source['base_descriptor_canonical_sha256'], 'source.base_descriptor_canonical_sha256'))};"
    )
    push(
        "inline constexpr char kSourceSchemaId[] = "
        f"{cpp_string(require_str(source['schema_id'], 'source.schema_id'))};"
    )
    push(
        "inline constexpr std::int32_t kSourceSchemaVersion = "
        f"{require_int(source['schema_version'], 'source.schema_version')};"
    )
    push("")
    movement_classes = pack["movement_classes"]
    if len(movement_classes) != 1:
        raise EmitError("exactly one movement class (ground) is expected")
    ground = movement_classes[0]
    push(
        "inline constexpr std::uint8_t kGroundMovementBit = "
        f"{require_int(ground['bit'], 'movement_classes[0].bit')};"
    )
    push(
        "inline constexpr std::uint8_t kGroundMovementMask = "
        f"{require_int(ground['mask'], 'movement_classes[0].mask')};"
    )
    push("inline constexpr std::int32_t kPassableBaseMoveCost = 10;")
    push("inline constexpr std::int32_t kBlockedBaseMoveCost = 0;")
    push(f"inline constexpr std::int32_t kExpectedBlockedCellCount = {EXPECTED_BLOCKED_CELLS};")
    push(f"inline constexpr std::int32_t kExpectedPassableCellCount = {EXPECTED_PASSABLE_CELLS};")
    push("")
    push("struct FCompiledHeightBand")
    push("{")
    push("    std::uint8_t Ordinal;")
    push("    const char* Id;")
    push("    std::int32_t RelativeLevel;")
    push("};")
    push("")
    bands = pack["height_bands"]
    push(f"inline constexpr FCompiledHeightBand kHeightBands[{len(bands)}] = {{")
    for band in bands:
        push(
            "    {"
            f"{require_int(band['ordinal'], 'height_band.ordinal')}, "
            f"{cpp_string(require_str(band['id'], 'height_band.id'))}, "
            f"{require_int(band['relative_level'], 'height_band.relative_level')}"
            "},"
        )
    push("};")
    push("")
    push("struct FCompiledRegion")
    push("{")
    push("    std::uint8_t Ordinal;")
    push("    const char* Id;")
    push("    std::int32_t CellCount;")
    push("    std::uint8_t HeightBandOrdinal;")
    push("};")
    push("")
    regions = pack["regions"]
    push(f"inline constexpr FCompiledRegion kRegions[{len(regions)}] = {{")
    for record in regions:
        push(
            "    {"
            f"{require_int(record['ordinal'], 'region.ordinal')}, "
            f"{cpp_string(require_str(record['id'], 'region.id'))}, "
            f"{require_int(record['cell_count'], 'region.cell_count')}, "
            f"{require_int(record['height_band_ordinal'], 'region.height_band_ordinal')}"
            "},"
        )
    push("};")
    push("")
    push("struct FCompiledPortal")
    push("{")
    push("    std::uint8_t Ordinal;")
    push("    const char* Id;")
    push("    std::uint8_t RegionOrdinalA;")
    push("    std::uint8_t RegionOrdinalB;")
    push("    std::int32_t EdgePairCount;")
    push("};")
    push("")
    portals = pack["portals"]
    edge_pairs: list[tuple[int, int, int]] = []
    portal_lines: list[str] = []
    for portal in portals:
        ordinal = require_int(portal["ordinal"], "portal.ordinal")
        pairs = portal["edge_index_pairs"]
        if not isinstance(pairs, list) or not pairs:
            raise EmitError(f"portal {portal.get('id')!r} has no edge pairs")
        region_pair = portal["region_ordinals"]
        if not isinstance(region_pair, list) or len(region_pair) != 2:
            raise EmitError(f"portal {portal.get('id')!r} region pair malformed")
        for pair in pairs:
            if not isinstance(pair, list) or len(pair) != 2:
                raise EmitError(f"portal {portal.get('id')!r} edge pair malformed")
            edge_pairs.append(
                (
                    ordinal,
                    require_int(pair[0], "portal edge index"),
                    require_int(pair[1], "portal edge index"),
                )
            )
        portal_lines.append(
            "    {"
            f"{ordinal}, "
            f"{cpp_string(require_str(portal['id'], 'portal.id'))}, "
            f"{require_int(region_pair[0], 'portal.region_ordinals[0]')}, "
            f"{require_int(region_pair[1], 'portal.region_ordinals[1]')}, "
            f"{len(pairs)}"
            "},"
        )
    push(f"inline constexpr FCompiledPortal kPortals[{len(portals)}] = {{")
    lines.extend(portal_lines)
    push("};")
    push("")
    push("struct FCompiledPortalEdgePair")
    push("{")
    push("    std::uint8_t PortalOrdinal;")
    push("    std::int16_t FirstCellIndex;")
    push("    std::int16_t SecondCellIndex;")
    push("};")
    push("")
    push(f"inline constexpr std::int32_t kPortalEdgePairCount = {len(edge_pairs)};")
    push("inline constexpr FCompiledPortalEdgePair kPortalEdgePairs[kPortalEdgePairCount] = {")
    for ordinal, first, second in edge_pairs:
        push(f"    {{{ordinal}, {first}, {second}}},")
    push("};")
    push("")
    objectives = pack["objectives"]
    if len(objectives) != 1:
        raise EmitError("exactly one objective (future-well) is expected")
    objective = objectives[0]
    if require_str(objective["id"], "objective.id") != "future-well":
        raise EmitError("unexpected objective id")
    fallbacks = objective["fallbacks"]
    push(
        "inline constexpr std::int32_t kFutureWellPrimaryCellIndex = "
        f"{require_int(objective['primary_index'], 'objective.primary_index')};"
    )
    push(
        f"inline constexpr std::int32_t kFutureWellFallbackCellIndices[{len(fallbacks)}] = {{"
    )
    ordered = sorted(
        fallbacks, key=lambda entry: require_int(entry["ordinal"], "fallback.ordinal")
    )
    push(
        "    "
        + ", ".join(
            str(require_int(entry["index"], "fallback.index")) for entry in ordered
        )
        + ","
    )
    push("};")
    push(
        "inline constexpr char kFutureWellFallbackPolicy[] = "
        f"{cpp_string(require_str(objective['fallback_policy'], 'objective.fallback_policy'))};"
    )
    push("")
    push("// Half-open tile-edge camera bounds: [minX, maxX) x [minY, maxY).")
    push(f"inline constexpr std::int32_t kCameraMinX = {require_int(camera['min_x'], 'camera.min_x')};")
    push(f"inline constexpr std::int32_t kCameraMinY = {require_int(camera['min_y'], 'camera.min_y')};")
    push(
        "inline constexpr std::int32_t kCameraMaxXExclusive = "
        f"{require_int(camera['max_x_exclusive'], 'camera.max_x_exclusive')};"
    )
    push(
        "inline constexpr std::int32_t kCameraMaxYExclusive = "
        f"{require_int(camera['max_y_exclusive'], 'camera.max_y_exclusive')};"
    )
    push("")
    lines.extend(format_array("kMovementMask", mask))
    push("")
    lines.extend(format_array("kBaseMoveCost", cost))
    push("")
    lines.extend(format_array("kHeightBandOrdinal", height))
    push("")
    lines.extend(format_array("kRegionOrdinal", region))
    push("")
    push("static_assert(sizeof(kMovementMask) == kCellCount);")
    push("static_assert(sizeof(kBaseMoveCost) == kCellCount);")
    push("static_assert(sizeof(kHeightBandOrdinal) == kCellCount);")
    push("static_assert(sizeof(kRegionOrdinal) == kCellCount);")
    push("")
    push("}  // namespace echoes::world::glass_scar_pack")
    return "\n".join(lines) + "\n"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Project root containing Content/ and Source/ (default: current directory).",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--check",
        action="store_true",
        help="Verify the checked-in header matches the fixture (default mode).",
    )
    mode.add_argument(
        "--write",
        action="store_true",
        help="Rewrite the generated header from the fixture.",
    )
    mode.add_argument(
        "--stdout",
        action="store_true",
        help="Render the header to standard output without touching any file.",
    )
    args = parser.parse_args(argv)

    repo_root = args.repo_root.resolve()
    try:
        pack, digest = load_pack(repo_root)
        rendered = render_header(pack, digest)
    except EmitError as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 1

    header_path = repo_root / HEADER_RELATIVE_PATH
    if args.stdout:
        sys.stdout.write(rendered)
        return 0
    if args.write:
        header_path.parent.mkdir(parents=False, exist_ok=True)
        header_path.write_text(rendered, encoding="utf-8", newline="\n")
        print(f"wrote {header_path} ({len(rendered)} bytes) digest={digest}")
        return 0
    if not header_path.is_file():
        print(f"REFUSED: checked-in header missing: {header_path}", file=sys.stderr)
        return 1
    existing = header_path.read_text(encoding="utf-8")
    if existing != rendered:
        print(
            "REFUSED: checked-in header does not match the fixture; "
            "regenerate with --write and re-review",
            file=sys.stderr,
        )
        return 1
    print(f"header matches fixture; digest={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
