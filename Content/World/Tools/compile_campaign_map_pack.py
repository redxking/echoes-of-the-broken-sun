#!/usr/bin/env python3
"""Compile the terrain-only campaign-map contract pack.

The compiler deliberately owns only source-contract identity.  It accepts a
manifest with the complete M01--M15 roster and one terrain-only source per
mission, refuses ambiguity or invalid topology, and emits deterministic data
for a runtime adapter. It does not select a map at runtime, modify save data,
or establish that a map is playable or presented in-game.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import tempfile
from collections import deque
from pathlib import Path, PurePosixPath
from typing import Any

GRID_WIDTH = 64
GRID_HEIGHT = 64
CELL_COUNT = GRID_WIDTH * GRID_HEIGHT
TILE_RAW = 1024
MISSION_CODES = tuple(f"M{number:02d}" for number in range(1, 16))
DOCTRINES = ("Harvest", "Preserve", "Reshape")
DOCTRINE_MASKS = {"Harvest": 1, "Preserve": 2, "Reshape": 4}
# StartScenario's ordinary campaign force.  The rectangles are the maximum
# occupied tile ranges for every candidate faction's current catalog archetype:
# a command core is 5x5, a barracks 4x4, and the remaining ordinary entities
# occupy their 2x2 terrain cells.  They deliberately model footprint terrain
# clearance, not entity-v-entity overlap; the latter is an existing runtime
# placement concern and is validated separately by the adapter.
ORDINARY_SCENARIO_DEPLOYMENTS = (
    ("local", "command-core", 7, 12, 7, 12),
    ("local", "barracks", 12, 15, 8, 11),
    ("local", "dropoff", 5, 6, 16, 17),
    ("local", "worker-a", 7, 8, 12, 13),
    ("local", "worker-b", 10, 11, 13, 14),
    ("local", "worker-c", 13, 14, 11, 12),
    ("local", "soldier-a", 7, 8, 7, 8),
    ("local", "soldier-b", 11, 12, 6, 7),
    ("local", "soldier-c", 15, 16, 9, 10),
    ("local", "heavy-unit", 6, 7, 5, 6),
    ("local", "scout-unit", 14, 15, 5, 6),
    ("local", "utility-structure", 5, 6, 10, 11),
    ("opponent", "command-core", 51, 56, 51, 56),
    ("opponent", "barracks", 48, 51, 52, 55),
    ("opponent", "dropoff", 57, 58, 47, 48),
    ("opponent", "worker-a", 50, 51, 52, 53),
    ("opponent", "worker-b", 53, 54, 49, 50),
    ("opponent", "worker-c", 56, 57, 51, 52),
    ("opponent", "soldier-a", 49, 50, 56, 57),
    ("opponent", "soldier-b", 53, 54, 57, 58),
    ("opponent", "heavy-unit", 56, 57, 57, 58),
    ("opponent", "scout-unit", 48, 49, 57, 58),
    ("opponent", "utility-structure", 57, 58, 52, 53),
)
OPERATION_MODES = (
    "CampaignPrologue",
    "CampaignSevenAccounts",
    "CampaignCityReserve",
    "CampaignUnburiedRoad",
    "CampaignTermsOfContinuance",
    "CampaignNamesWithoutBirths",
    "CampaignShapeOfSilence",
    "CampaignShapeBesideUs",
    "CampaignReserveAuthority",
    "CampaignChoirAtLumeReach",
    "CampaignNoNeutralLedger",
    "CampaignFutureThatWon",
    "CampaignAssemblyOfTheMissing",
    "CampaignSeveralVoicesOneCommand",
    "CampaignTheBrokenSun",
)
if len(MISSION_CODES) != len(OPERATION_MODES):
    raise RuntimeError("campaign operation roster length mismatch")
OPERATION_BY_MISSION = dict(zip(MISSION_CODES, OPERATION_MODES))
ID_PATTERN = re.compile(r"^[a-z][a-z0-9-]{0,63}$")
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")

DEFAULT_MANIFEST = Path("Content/World/Source/Campaign/campaign_map_manifest_v1.json")
DEFAULT_OUTPUT_DIR = Path("Content/World/Generated/Campaign")
DEFAULT_HEADER = Path("Content/World/Generated/Campaign/EchoesCampaignMapPack.h")
DEFAULT_REGISTRY = Path("Content/World/Generated/Campaign/campaign_map_registry_v1.json")


class CompileError(RuntimeError):
    """Raised for every fail-closed contract error."""


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise CompileError(f"duplicate JSON object key: {key!r}")
        result[key] = value
    return result


def reject_nonfinite(value: str) -> None:
    raise CompileError(f"non-finite JSON constant rejected: {value}")


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def load_strict_json(path: Path) -> tuple[dict[str, Any], str]:
    if not path.is_file():
        raise CompileError(f"input missing or not a regular file: {path}")
    raw = path.read_bytes()
    try:
        value = json.loads(raw.decode("utf-8"), object_pairs_hook=reject_duplicate_keys, parse_constant=reject_nonfinite)
    except UnicodeDecodeError as error:
        raise CompileError(f"{path}: invalid UTF-8 JSON: {error}") from error
    except json.JSONDecodeError as error:
        raise CompileError(f"{path}: invalid JSON at line {error.lineno}, column {error.colno}") from error
    if not isinstance(value, dict):
        raise CompileError(f"{path}: root must be an object")
    return value, sha256_bytes(raw)


def exact_keys(value: Any, required: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise CompileError(f"{label}: must be an object")
    actual = set(value)
    if actual != required:
        raise CompileError(f"{label}: exact-key mismatch (unexpected: {sorted(actual-required)}; missing: {sorted(required-actual)})")
    return value


def require_str(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value:
        raise CompileError(f"{label}: must be a non-empty string")
    return value


def require_int(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise CompileError(f"{label}: must be an integer")
    return value


def require_id(value: Any, label: str) -> str:
    text = require_str(value, label)
    if not ID_PATTERN.fullmatch(text):
        raise CompileError(f"{label}: invalid stable ID {text!r}")
    return text


def require_sha256(value: Any, label: str) -> str:
    text = require_str(value, label)
    if not SHA256_PATTERN.fullmatch(text):
        raise CompileError(f"{label}: must be a lowercase SHA-256 digest")
    return text


def safe_campaign_source(repo_root: Path, declared: Any, label: str) -> tuple[Path, str]:
    text = require_str(declared, label)
    pure = PurePosixPath(text)
    prefix = PurePosixPath("Content/World/Source/Campaign")
    if pure.is_absolute() or ".." in pure.parts or not pure.is_relative_to(prefix) or pure == prefix or pure.suffix != ".json":
        raise CompileError(f"{label}: path must be a .json file under {prefix.as_posix()}/")
    path = repo_root.joinpath(*pure.parts)
    current = repo_root
    for part in pure.parts:
        current = current / part
        if current.is_symlink():
            raise CompileError(f"{label}: symlink path component refused: {current}")
    if not path.is_file():
        raise CompileError(f"{label}: source file missing: {text}")
    resolved_root = repo_root.resolve()
    resolved = path.resolve(strict=True)
    if not resolved.is_relative_to(resolved_root):
        raise CompileError(f"{label}: resolved path escapes repository")
    return resolved, pure.as_posix()


def apply_ops(base: set[int], ops: Any, label: str) -> set[int]:
    if not isinstance(ops, list):
        raise CompileError(f"{label}: must be an array")
    blocked = set(base)
    seen_ids: set[str] = set()
    for position, value in enumerate(ops):
        op = exact_keys(value, {"id", "op", "x0", "x1", "y0", "y1"}, f"{label}[{position}]")
        op_id = require_id(op["id"], f"{label}[{position}].id")
        if op_id in seen_ids:
            raise CompileError(f"{label}: duplicate region operation ID {op_id!r}")
        seen_ids.add(op_id)
        kind = require_str(op["op"], f"{label}[{position}].op")
        if kind not in ("block", "open"):
            raise CompileError(f"{label}[{position}].op: expected block or open")
        x0, x1 = require_int(op["x0"], f"{label}[{position}].x0"), require_int(op["x1"], f"{label}[{position}].x1")
        y0, y1 = require_int(op["y0"], f"{label}[{position}].y0"), require_int(op["y1"], f"{label}[{position}].y1")
        if not (0 <= x0 <= x1 < GRID_WIDTH and 0 <= y0 <= y1 < GRID_HEIGHT):
            raise CompileError(f"{label}[{position}]: rectangle out of bounds or inverted")
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                index = y * GRID_WIDTH + x
                if kind == "block":
                    blocked.add(index)
                else:
                    blocked.discard(index)
    return blocked


def reachable(blocked: set[int], start: int) -> set[int]:
    if start in blocked:
        return set()
    seen = {start}
    queue = deque((start,))
    while queue:
        index = queue.popleft()
        x, y = index % GRID_WIDTH, index // GRID_WIDTH
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < GRID_WIDTH and 0 <= ny < GRID_HEIGHT:
                neighbor = ny * GRID_WIDTH + nx
                if neighbor not in blocked and neighbor not in seen:
                    seen.add(neighbor)
                    queue.append(neighbor)
    return seen


def require_connected(blocked: set[int], label: str) -> None:
    passable = CELL_COUNT - len(blocked)
    if passable == 0:
        raise CompileError(f"{label}: terrain contains no walkable cells")
    first = next(index for index in range(CELL_COUNT) if index not in blocked)
    if len(reachable(blocked, first)) != passable:
        raise CompileError(f"{label}: walkable terrain is disconnected")


def parse_grid(value: Any, label: str) -> None:
    grid = exact_keys(value, {"width_tiles", "height_tiles", "index_formula", "coordinate_origin"}, label)
    if (require_int(grid["width_tiles"], f"{label}.width_tiles") != GRID_WIDTH or
            require_int(grid["height_tiles"], f"{label}.height_tiles") != GRID_HEIGHT or
            require_str(grid["index_formula"], f"{label}.index_formula") != "y*width+x" or
            require_str(grid["coordinate_origin"], f"{label}.coordinate_origin") != "southwest"):
        raise CompileError(f"{label}: requires the fixed 64x64 southwest row-major grid")


def parse_assertions(value: Any, blocked: set[int], label: str) -> None:
    if value is None:
        return
    if not isinstance(value, list):
        raise CompileError(f"{label}: must be an array when present")
    ids: set[str] = set()
    for position, item in enumerate(value):
        point = exact_keys(item, {"id", "x", "y"}, f"{label}[{position}]")
        point_id = require_id(point["id"], f"{label}[{position}].id")
        if point_id in ids:
            raise CompileError(f"{label}: duplicate assertion ID {point_id!r}")
        ids.add(point_id)
        x, y = require_int(point["x"], f"{label}[{position}].x"), require_int(point["y"], f"{label}[{position}].y")
        if not (0 <= x < GRID_WIDTH and 0 <= y < GRID_HEIGHT):
            raise CompileError(f"{label}[{position}]: assertion outside grid")
        if y * GRID_WIDTH + x in blocked:
            raise CompileError(f"{label}[{position}]: required-passable assertion is blocked")


def parse_clearance(value: Any, blocked: set[int], label: str) -> None:
    if not isinstance(value, list):
        raise CompileError(f"{label}: must be an array")
    ids: set[str] = set()
    for position, item in enumerate(value):
        clearance = exact_keys(item, {"id", "x", "y", "half_extent_raw"}, f"{label}[{position}]")
        clearance_id = require_id(clearance["id"], f"{label}[{position}].id")
        if clearance_id in ids:
            raise CompileError(f"{label}: duplicate clearance ID {clearance_id!r}")
        ids.add(clearance_id)
        x = require_int(clearance["x"], f"{label}[{position}].x")
        y = require_int(clearance["y"], f"{label}[{position}].y")
        half_extent = require_int(clearance["half_extent_raw"], f"{label}[{position}].half_extent_raw")
        if not 1 <= half_extent <= TILE_RAW:
            raise CompileError(f"{label}[{position}].half_extent_raw: must be in 1..{TILE_RAW}")
        x0 = (x * TILE_RAW - half_extent) // TILE_RAW
        x1 = (x * TILE_RAW + half_extent - 1) // TILE_RAW
        y0 = (y * TILE_RAW - half_extent) // TILE_RAW
        y1 = (y * TILE_RAW + half_extent - 1) // TILE_RAW
        if not (0 <= x0 <= x1 < GRID_WIDTH and 0 <= y0 <= y1 < GRID_HEIGHT):
            raise CompileError(f"{label}[{position}]: footprint leaves the terrain grid")
        for clearance_y in range(y0, y1 + 1):
            for clearance_x in range(x0, x1 + 1):
                if clearance_y * GRID_WIDTH + clearance_x in blocked:
                    raise CompileError(
                        f"{label}[{position}]: required footprint intersects blocked terrain "
                        f"at ({clearance_x},{clearance_y})"
                    )


def require_common_scenario_spawns(blocked: set[int], label: str) -> None:
    for team, entity, x0, x1, y0, y1 in ORDINARY_SCENARIO_DEPLOYMENTS:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if y * GRID_WIDTH + x in blocked:
                    raise CompileError(
                        f"{label}: StartScenario {team} {entity} footprint intersects "
                        f"blocked terrain at ({x},{y})"
                    )


def parse_source(source: dict[str, Any], source_digest: str, manifest_row: dict[str, Any], label: str) -> dict[str, Any]:
    required = {"source_format", "source_version", "author", "mission_code", "map_id", "operation_mode", "production_brief", "grid", "base_terrain", "terrain_region_ops", "founding_doctrine_variants", "required_clearance"}
    allowed = required | {"required_passable"}
    if set(source) - allowed or required - set(source):
        raise CompileError(f"{label}: exact-key mismatch (unexpected: {sorted(set(source)-allowed)}; missing: {sorted(required-set(source))})")
    if require_str(source["source_format"], f"{label}.source_format") != "echoes-campaign-map-source":
        raise CompileError(f"{label}: unexpected source_format")
    if require_int(source["source_version"], f"{label}.source_version") != 1:
        raise CompileError(f"{label}: unexpected source_version")
    if require_str(source["author"], f"{label}.author") != "Angelis Pseftis":
        raise CompileError(f"{label}: author must be Angelis Pseftis")
    for key in ("mission_code", "map_id", "operation_mode"):
        if source[key] != manifest_row[key]:
            raise CompileError(f"{label}.{key}: does not match manifest")
    brief = exact_keys(source["production_brief"], {"mission_requirement", "map_concepts_reference"}, f"{label}.production_brief")
    expected_msn = f"SPEC-MSN-{int(manifest_row['mission_code'][1:]):03d}"
    if require_str(brief["mission_requirement"], f"{label}.production_brief.mission_requirement") != expected_msn:
        raise CompileError(f"{label}: production brief must reference {expected_msn}")
    if require_str(brief["map_concepts_reference"], f"{label}.production_brief.map_concepts_reference") != f"MapConcepts#{manifest_row['mission_code']}":
        raise CompileError(f"{label}: production brief must reference its MapConcepts mission anchor")
    parse_grid(source["grid"], f"{label}.grid")
    base_terrain = require_str(source["base_terrain"], f"{label}.base_terrain")
    if base_terrain not in ("open", "blocked"):
        raise CompileError(f"{label}.base_terrain: expected open or blocked")
    base = set() if base_terrain == "open" else set(range(CELL_COUNT))
    common = apply_ops(base, source["terrain_region_ops"], f"{label}.terrain_region_ops")
    variants = source["founding_doctrine_variants"]
    if not isinstance(variants, list) or len(variants) != len(DOCTRINES):
        raise CompileError(f"{label}.founding_doctrine_variants: requires exactly Harvest, Preserve, Reshape")
    compiled_variants: list[dict[str, Any]] = []
    seen: set[str] = set()
    for position, value in enumerate(variants):
        variant = exact_keys(value, {"doctrine", "terrain_region_ops", "expected_blocked_cell_count"}, f"{label}.founding_doctrine_variants[{position}]")
        doctrine = require_str(variant["doctrine"], f"{label}.founding_doctrine_variants[{position}].doctrine")
        if doctrine not in DOCTRINES or doctrine in seen:
            raise CompileError(f"{label}.founding_doctrine_variants: must contain each doctrine exactly once")
        seen.add(doctrine)
        blocked = apply_ops(common, variant["terrain_region_ops"], f"{label}.{doctrine}.terrain_region_ops")
        expected_blocked = require_int(variant["expected_blocked_cell_count"], f"{label}.{doctrine}.expected_blocked_cell_count")
        if len(blocked) != expected_blocked:
            raise CompileError(f"{label}.{doctrine}: blocked-cell census {len(blocked)} diverges from declared {expected_blocked}")
        require_connected(blocked, f"{label}.{doctrine}")
        parse_assertions(source.get("required_passable"), blocked, f"{label}.required_passable")
        parse_clearance(source["required_clearance"], blocked, f"{label}.required_clearance")
        require_common_scenario_spawns(blocked, f"{label}.{doctrine}")
        compiled_variants.append({"doctrine": doctrine, "movement_mask": [0 if index in blocked else 1 for index in range(CELL_COUNT)], "blocked_cell_count": len(blocked)})
    if tuple(item["doctrine"] for item in compiled_variants) != DOCTRINES:
        raise CompileError(f"{label}.founding_doctrine_variants: canonical order must be {list(DOCTRINES)}")
    terrain_identity = sha256_bytes(canonical_bytes(compiled_variants))
    anchors = [item for item in source["required_clearance"] if item["id"] == "future-well"]
    return {"source_sha256": source_digest, "terrain_identity_sha256": terrain_identity, "variants": compiled_variants, "anchors": anchors}


def parse_manifest(manifest: dict[str, Any], repo_root: Path) -> list[dict[str, Any]]:
    top = exact_keys(manifest, {"manifest_format", "manifest_version", "author", "mission_maps"}, "manifest")
    if require_str(top["manifest_format"], "manifest.manifest_format") != "echoes-campaign-map-manifest":
        raise CompileError("manifest: unexpected manifest_format")
    if require_int(top["manifest_version"], "manifest.manifest_version") != 1:
        raise CompileError("manifest: unexpected manifest_version")
    if require_str(top["author"], "manifest.author") != "Angelis Pseftis":
        raise CompileError("manifest: author must be Angelis Pseftis")
    rows = top["mission_maps"]
    if not isinstance(rows, list) or len(rows) != len(MISSION_CODES):
        raise CompileError("manifest.mission_maps: requires exactly fifteen ordered mission rows")
    parsed: list[dict[str, Any]] = []
    map_ids: set[str] = set()
    sources: set[str] = set()
    for position, value in enumerate(rows):
        row = exact_keys(value, {"mission_code", "map_id", "operation_mode", "source_path", "source_sha256"}, f"manifest.mission_maps[{position}]")
        mission = require_str(row["mission_code"], f"manifest.mission_maps[{position}].mission_code")
        if mission != MISSION_CODES[position]:
            raise CompileError(f"manifest.mission_maps[{position}]: expected ordered mission {MISSION_CODES[position]}")
        map_id = require_id(row["map_id"], f"manifest.mission_maps[{position}].map_id")
        if map_id in map_ids:
            raise CompileError(f"manifest: duplicate map ID {map_id!r}")
        map_ids.add(map_id)
        operation = require_str(row["operation_mode"], f"manifest.mission_maps[{position}].operation_mode")
        if operation != OPERATION_BY_MISSION[mission]:
            raise CompileError(f"manifest: {mission} must use existing operation enum {OPERATION_BY_MISSION[mission]}")
        source_path, source_text = safe_campaign_source(repo_root, row["source_path"], f"manifest.mission_maps[{position}].source_path")
        if source_text in sources:
            raise CompileError(f"manifest: duplicate source path {source_text!r}")
        sources.add(source_text)
        pinned = require_sha256(row["source_sha256"], f"manifest.mission_maps[{position}].source_sha256")
        source, actual = load_strict_json(source_path)
        if actual != pinned:
            raise CompileError(f"manifest: {mission} source SHA-256 mismatch (computed {actual}, pinned {pinned})")
        parsed_row = dict(row)
        parsed_row["source_path"] = source_text
        parsed_row.update(parse_source(source, actual, parsed_row, f"{source_text}"))
        parsed.append(parsed_row)
    identities = [row["terrain_identity_sha256"] for row in parsed]
    if len(identities) != len(set(identities)):
        raise CompileError("manifest: two missions compile to the same complete terrain layout")
    return parsed


def render_map(row: dict[str, Any]) -> bytes:
    output = {
        "pack_format": "echoes-compiled-campaign-map",
        "pack_version": 1,
        "runtime_binding": "EchoesCampaignTerrainBinding-source-wired-execution-unverified",
        "mission_code": row["mission_code"],
        "map_id": row["map_id"],
        "operation_mode": row["operation_mode"],
        "source_contract": {"source_path": row["source_path"], "source_sha256": row["source_sha256"]},
        "grid": {"width_tiles": GRID_WIDTH, "height_tiles": GRID_HEIGHT, "index_formula": "y*width+x", "coordinate_origin": "southwest"},
        "founding_doctrine_variants": row["variants"],
        "mission_anchors": row["anchors"],
        "terrain_identity_sha256": row["terrain_identity_sha256"],
        "claim_boundary": {"runtime_binding": "source-wired-execution-unverified", "not_evidence_for": ["successful-runtime-selection", "save-format-compatibility", "playability", "rendered-presentation"]},
    }
    return canonical_bytes(output) + b"\n"


def cpp_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def render_header(rows: list[dict[str, Any]], registry_digest: str) -> bytes:
    def array_name(mission_code: str, doctrine: str) -> str:
        return f"k{mission_code}{doctrine}BlockedCells"

    lines = [
        "// GENERATED FILE - do not edit by hand.",
        "// Source: Content/World/Tools/compile_campaign_map_pack.py",
        "#pragma once", "", "#include <array>", "#include <cstdint>", "#include <string_view>", "",
        "namespace echoes::world::campaign_map_pack", "{", "",
        f"inline constexpr std::int32_t kGridWidthTiles = {GRID_WIDTH};", f"inline constexpr std::int32_t kGridHeightTiles = {GRID_HEIGHT};", f"inline constexpr std::int32_t kCellCount = {CELL_COUNT};", "",
        "struct MissionIdentity final", "{", "    std::string_view mission_code;", "    std::string_view map_id;", "    std::string_view operation_mode;", "    std::string_view source_sha256;", "    std::string_view terrain_identity_sha256;", "    std::uint8_t doctrine_variant_mask;", "};", "",
        "struct TerrainVariant final", "{", "    std::uint8_t mission_index;", "    std::uint8_t doctrine_mask;", "    std::uint16_t expected_blocked_cell_count;", "    const std::array<std::uint8_t, kCellCount>* blocked_cells;", "};", "",
        "struct MissionAnchor final { std::uint8_t mission_ordinal; std::string_view id; std::int32_t x, y; };", "",
        f"inline constexpr std::string_view kRegistrySha256 = {cpp_string(registry_digest)};",
        "inline constexpr std::uint8_t kHarvestMask = 0x01;", "inline constexpr std::uint8_t kPreserveMask = 0x02;", "inline constexpr std::uint8_t kReshapeMask = 0x04;", "",
        "inline constexpr std::array<MissionIdentity, 15> kMissionIdentities = {{",
    ]
    for row in rows:
        lines.append("    {" + ", ".join((cpp_string(row["mission_code"]), cpp_string(row["map_id"]), cpp_string(row["operation_mode"]), cpp_string(row["source_sha256"]), cpp_string(row["terrain_identity_sha256"]), "0x07")) + "},")
    lines.extend(["}};", ""])
    anchors = [(index + 1, anchor) for index, row in enumerate(rows) for anchor in row["anchors"]]
    lines.append(f"inline constexpr std::array<MissionAnchor, {len(anchors)}> kMissionAnchors = {{{{")
    for ordinal, anchor in anchors:
        lines.append(f"    {{{ordinal}, {cpp_string(anchor['id'])}, {anchor['x']}, {anchor['y']}}},")
    lines.extend(["}};", ""])
    for row in rows:
        for variant in row["variants"]:
            blocked = [1 - value for value in variant["movement_mask"]]
            lines.append(f"inline constexpr std::array<std::uint8_t, kCellCount> {array_name(row['mission_code'], variant['doctrine'])} = {{{{")
            for start in range(0, CELL_COUNT, 32):
                lines.append("    " + ",".join(str(value) for value in blocked[start:start + 32]) + ",")
            lines.extend(["}};", ""])
    lines.append("inline constexpr std::array<TerrainVariant, 45> kTerrainVariants = {{")
    for mission_index, row in enumerate(rows):
        for variant in row["variants"]:
            lines.append(f"    {{{mission_index}, 0x{DOCTRINE_MASKS[variant['doctrine']]:02x}, {variant['blocked_cell_count']}, &{array_name(row['mission_code'], variant['doctrine'])}}},")
    lines.extend(["}};", "", "} // namespace echoes::world::campaign_map_pack", ""])
    return "\n".join(lines).encode("utf-8")


def compile_outputs(repo_root: Path, manifest_path: Path, output_dir: Path, header_path: Path, registry_path: Path) -> dict[Path, bytes]:
    manifest, _ = load_strict_json(manifest_path)
    rows = parse_manifest(manifest, repo_root)
    outputs: dict[Path, bytes] = {}
    registry_rows: list[dict[str, Any]] = []
    for row in rows:
        content = render_map(row)
        map_path = output_dir / f"{row['mission_code'].lower()}_{row['map_id']}_v1.json"
        outputs[map_path] = content
        registry_rows.append({"mission_code": row["mission_code"], "map_id": row["map_id"], "operation_mode": row["operation_mode"], "source_path": row["source_path"], "source_sha256": row["source_sha256"], "compiled_path": map_path.name, "compiled_sha256": sha256_bytes(content), "terrain_identity_sha256": row["terrain_identity_sha256"], "doctrine_variant_mask": 7})
    registry = {"registry_format": "echoes-campaign-map-registry", "registry_version": 1, "runtime_binding": "EchoesCampaignTerrainBinding-source-wired-execution-unverified", "mission_maps": registry_rows}
    registry_content = canonical_bytes(registry) + b"\n"
    outputs[registry_path] = registry_content
    outputs[header_path] = render_header(rows, sha256_bytes(registry_content))
    return outputs


def atomic_write_all(outputs: dict[Path, bytes]) -> None:
    staged: list[tuple[Path, Path]] = []
    try:
        for destination, content in outputs.items():
            destination.parent.mkdir(parents=True, exist_ok=True)
            descriptor, temporary = tempfile.mkstemp(prefix=f".{destination.name}.", dir=destination.parent)
            with os.fdopen(descriptor, "wb") as handle:
                handle.write(content)
                handle.flush()
                os.fsync(handle.fileno())
            staged.append((Path(temporary), destination))
        for temporary, destination in staged:
            os.replace(temporary, destination)
    finally:
        for temporary, _ in staged:
            if temporary.exists():
                temporary.unlink()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--header", type=Path, default=DEFAULT_HEADER)
    parser.add_argument("--registry", type=Path, default=DEFAULT_REGISTRY)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--write", action="store_true", help="atomically write every output after validation")
    mode.add_argument("--check", action="store_true", help="fail if an output is missing or stale (default)")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    resolve = lambda path: path if path.is_absolute() else root / path
    try:
        outputs = compile_outputs(root, resolve(args.manifest), resolve(args.output_dir), resolve(args.header), resolve(args.registry))
        if args.write:
            atomic_write_all(outputs)
            print(f"campaign map pack written: {len(outputs)} outputs")
            return 0
        stale = [str(path) for path, content in outputs.items() if not path.is_file() or path.read_bytes() != content]
        if stale:
            raise CompileError("campaign map outputs missing or stale: " + ", ".join(stale))
        print(f"campaign map pack current: {len(outputs)} outputs")
        return 0
    except CompileError as error:
        print(f"campaign map compilation refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
