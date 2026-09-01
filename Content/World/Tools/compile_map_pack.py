#!/usr/bin/env python3
"""Compile the proposed Glass Scar v2 source contract into canonical source data.

The compiled fixture has no runtime binding. This tool validates deterministic
source structure and graph invariants; it does not exercise Unreal navigation,
AI, rendering, gameplay, packaging, or performance.
"""

from __future__ import annotations

import argparse
import hashlib
import heapq
import json
import os
import re
import sys
import tempfile
import unicodedata
from collections import deque
from collections.abc import Callable, Iterable
from pathlib import Path, PurePosixPath
from typing import Any, NoReturn

PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_SOURCE = (
    PROJECT_ROOT
    / "Content/World/Source/GlassScar/glass_scar_map_source_v2.json"
)
DEFAULT_OUTPUT = (
    PROJECT_ROOT
    / "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
)
BASE_DESCRIPTOR_PATH = "Content/World/Source/GlassScar/glass_scar_map_pack_v1.json"
BASE_DESCRIPTOR_CANONICAL_SHA256 = (
    "6710b95a3a2e64acdfa044a1d0c302fff529daadd7d77d987d8c4e329098bf7d"
)

SOURCE_KEYS = {
    "schema_id",
    "schema_version",
    "map_id",
    "display_name",
    "authority",
    "runtime_binding",
    "base_descriptor",
    "canonicalization",
    "grid",
    "movement_classes",
    "height_bands",
    "regions",
    "blocked_zones",
    "portals",
    "objectives",
    "camera_bounds",
    "claim_boundary",
}

COMPILED_KEYS = {
    "pack_format",
    "pack_version",
    "schema_version",
    "map_id",
    "display_name",
    "authority",
    "runtime_binding",
    "source_contract",
    "canonicalization",
    "grid",
    "movement_classes",
    "height_bands",
    "regions",
    "blocked_zones",
    "portals",
    "cells",
    "objectives",
    "camera_bounds",
    "claim_boundary",
}

CANONICALIZATION: dict[str, Any] = {
    "profile": "echoes-compiled-json-v1",
    "algorithm": "sha256",
    "encoding": "utf-8",
    "object_keys": "lexicographic",
    "arrays": "contract-ordered",
    "whitespace": "none",
    "terminal_newline": False,
    "digest_scope": "exact-compiled-file-bytes",
}

SOURCE_CANONICALIZATION: dict[str, Any] = {
    **CANONICALIZATION,
    "output_file": "glass_scar_compiled_map_pack_v1.json",
    "digest_file": "glass_scar_compiled_map_pack_v1.sha256",
}

GRID_CONTRACT: dict[str, Any] = {
    "width_tiles": 64,
    "height_tiles": 64,
    "coordinate_origin": "southwest",
    "layout": "row-major-yx",
    "index_formula": "y*width+x",
    "movement_cost_semantics": "destination-tile-entry-cost",
    "simulation_tile_centimeters": 100,
    "presentation_tile_world_units": 200,
}

CLAIM_EXCLUSIONS = [
    "runtime-authority",
    "runtime-consumption",
    "network-compatibility",
    "current-movement-costs",
    "current-height-or-slope",
    "current-region-or-portal-routing",
    "objective-fallback-execution",
    "camera-clamp-or-readability",
    "unreal-navmesh",
    "ai-or-formation-traversal",
    "rendered-or-playable-map",
    "packaged-or-performance-qualified",
    "release-readiness",
]

REQUIRED_REGION_IDS = [
    "south-basin",
    "north-basin",
    "west-edge-corridor",
    "ash-cut",
    "buried-causeway",
    "folded-verge",
    "east-edge-corridor",
]

REQUIRED_REGION_COUNTS = {
    "south-basin": 1920,
    "north-basin": 1856,
    "west-edge-corridor": 40,
    "ash-cut": 20,
    "buried-causeway": 35,
    "folded-verge": 20,
    "east-edge-corridor": 40,
}

REQUIRED_REGION_GEOMETRY: dict[
    str, tuple[tuple[int, int, int, int], tuple[int, int]]
] = {
    "south-basin": ((0, 63, 0, 29), (10, 10)),
    "north-basin": ((0, 63, 35, 63), (54, 54)),
    "west-edge-corridor": ((0, 7, 30, 34), (7, 32)),
    "ash-cut": ((12, 15, 30, 34), (13, 32)),
    "buried-causeway": ((29, 35, 30, 34), (32, 32)),
    "folded-verge": ((48, 51, 30, 34), (49, 32)),
    "east-edge-corridor": ((56, 63, 30, 34), (56, 32)),
}

REQUIRED_BLOCKED_GEOMETRY: dict[str, tuple[tuple[int, int, int, int], int]] = {
    "scar-west-shoulder": ((8, 11, 30, 34), 20),
    "scar-west-span": ((16, 28, 30, 34), 65),
    "scar-east-span": ((36, 47, 30, 34), 60),
    "scar-east-shoulder": ((52, 55, 30, 34), 20),
}

REQUIRED_PORTAL_IDS = [
    "west-edge-south",
    "west-edge-north",
    "ash-cut-south",
    "ash-cut-north",
    "buried-causeway-south",
    "buried-causeway-north",
    "folded-verge-south",
    "folded-verge-north",
    "east-edge-south",
    "east-edge-north",
]

REQUIRED_PORTAL_REGION_IDS: dict[str, tuple[str, str]] = {
    "west-edge-south": ("south-basin", "west-edge-corridor"),
    "west-edge-north": ("west-edge-corridor", "north-basin"),
    "ash-cut-south": ("south-basin", "ash-cut"),
    "ash-cut-north": ("ash-cut", "north-basin"),
    "buried-causeway-south": ("south-basin", "buried-causeway"),
    "buried-causeway-north": ("buried-causeway", "north-basin"),
    "folded-verge-south": ("south-basin", "folded-verge"),
    "folded-verge-north": ("folded-verge", "north-basin"),
    "east-edge-south": ("south-basin", "east-edge-corridor"),
    "east-edge-north": ("east-edge-corridor", "north-basin"),
}

REQUIRED_OBJECTIVE_ID = "future-well"
REQUIRED_FUTURE_WELL_TILE = (32, 32)

CORRIDOR_ANCHORS: dict[str, tuple[tuple[int, int], tuple[int, int]]] = {
    "west-edge-corridor": ((7, 29), (7, 35)),
    "ash-cut": ((13, 29), (13, 35)),
    "buried-causeway": ((32, 29), (32, 35)),
    "folded-verge": ((49, 29), (49, 35)),
    "east-edge-corridor": ((56, 29), (56, 35)),
}

ID_PATTERN = re.compile(r"^[a-z][a-z0-9-]*$")
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")


class MapContractError(ValueError):
    """Raised when proposed authoring or compiled source data fails closed."""


def fail(path: str, message: str) -> NoReturn:
    raise MapContractError(f"{path}: {message}")


def strict_json_loads(text: str, path: str = "json") -> Any:
    def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                fail(path, f"duplicate object key '{key}'")
            result[key] = value
        return result

    def reject_nonstandard_constant(value: str) -> None:
        fail(path, f"non-standard numeric constant '{value}'")

    try:
        return json.loads(
            text,
            object_pairs_hook=reject_duplicate_keys,
            parse_constant=reject_nonstandard_constant,
        )
    except json.JSONDecodeError as error:
        fail(path, f"invalid JSON at line {error.lineno}, column {error.colno}")


def strict_json_load(path: Path) -> Any:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(str(path), f"cannot read UTF-8 JSON: {error}")
    return strict_json_loads(text, str(path))


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
        allow_nan=False,
    ).encode("utf-8")


def paths_collide(first: Path, second: Path) -> bool:
    if first.resolve() == second.resolve():
        return True
    try:
        return first.exists() and second.exists() and os.path.samefile(first, second)
    except OSError:
        return False


def remove_temporary(path: Path | None) -> None:
    if path is None:
        return
    try:
        path.unlink(missing_ok=True)
    except OSError:
        pass


def transactional_write_pair(
    first_path: Path,
    first_data: bytes,
    second_path: Path,
    second_data: bytes,
) -> None:
    destinations = ((first_path, first_data), (second_path, second_data))
    staged: dict[Path, Path] = {}
    backups: dict[Path, Path] = {}
    placed: set[Path] = set()

    try:
        for path, _ in destinations:
            path.parent.mkdir(parents=True, exist_ok=True)
            if path.is_symlink():
                fail(str(path), "write destination must not be a symbolic link")
            if path.exists() and not path.is_file():
                fail(str(path), "write destination must be a regular file")

        for path, data in destinations:
            descriptor, temporary_name = tempfile.mkstemp(
                prefix=f".{path.name}.staged.", dir=path.parent
            )
            temporary = Path(temporary_name)
            staged[path] = temporary
            existing_mode = path.stat().st_mode & 0o777 if path.exists() else 0o644
            with os.fdopen(descriptor, "wb") as stream:
                os.fchmod(stream.fileno(), existing_mode)
                stream.write(data)
                stream.flush()
                os.fsync(stream.fileno())

        for path, _ in destinations:
            if not path.exists():
                continue
            backup_descriptor, backup_name = tempfile.mkstemp(
                prefix=f".{path.name}.backup.", dir=path.parent
            )
            os.close(backup_descriptor)
            backup = Path(backup_name)
            try:
                os.replace(path, backup)
            except OSError:
                remove_temporary(backup)
                raise
            backups[path] = backup

        for path, _ in destinations:
            os.replace(staged[path], path)
            placed.add(path)
    except MapContractError:
        for temporary in staged.values():
            remove_temporary(temporary)
        for backup in backups.values():
            remove_temporary(backup)
        raise
    except OSError as error:
        rollback_errors: list[str] = []
        unrestored_backups: set[Path] = set()
        for path, _ in reversed(destinations):
            if path in placed:
                try:
                    path.unlink(missing_ok=True)
                except OSError as rollback_error:
                    rollback_errors.append(str(rollback_error))
            existing_backup = backups.get(path)
            if existing_backup is not None and existing_backup.exists():
                try:
                    os.replace(existing_backup, path)
                except OSError as rollback_error:
                    rollback_errors.append(str(rollback_error))
                    unrestored_backups.add(existing_backup)
        for temporary in staged.values():
            remove_temporary(temporary)
        for backup in backups.values():
            if backup not in unrestored_backups:
                remove_temporary(backup)
        suffix = ""
        if rollback_errors:
            retained = ", ".join(str(path) for path in sorted(unrestored_backups))
            suffix = (
                "; rollback errors: "
                + " | ".join(rollback_errors)
                + f"; retained backups: {retained}"
            )
        fail("output", f"transactional write failed: {error}{suffix}")
    else:
        for backup in backups.values():
            remove_temporary(backup)


def require_object(value: Any, path: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        fail(path, "must be an object")
    return value


def require_array(value: Any, path: str) -> list[Any]:
    if not isinstance(value, list):
        fail(path, "must be an array")
    return value


def require_exact_keys(value: dict[str, Any], expected: set[str], path: str) -> None:
    actual = set(value)
    missing = expected - actual
    extra = actual - expected
    if missing:
        fail(path, "missing fields: " + ", ".join(sorted(missing)))
    if extra:
        fail(path, "unknown fields: " + ", ".join(sorted(extra)))


def require_int(
    value: Any,
    path: str,
    minimum: int | None = None,
    maximum: int | None = None,
) -> int:
    if not isinstance(value, int) or isinstance(value, bool):
        fail(path, "must be an integer")
    if minimum is not None and value < minimum:
        fail(path, f"must be at least {minimum}")
    if maximum is not None and value > maximum:
        fail(path, f"must be at most {maximum}")
    return value


def require_bool(value: Any, path: str) -> bool:
    if not isinstance(value, bool):
        fail(path, "must be a boolean")
    return value


def require_text(value: Any, path: str) -> str:
    if not isinstance(value, str) or not value:
        fail(path, "must be a non-empty string")
    if unicodedata.normalize("NFC", value) != value:
        fail(path, "must use Unicode NFC normalization")
    if "\x00" in value:
        fail(path, "must not contain NUL")
    return value


def require_id(value: Any, path: str) -> str:
    text = require_text(value, path)
    if len(text) > 64 or ID_PATTERN.fullmatch(text) is None:
        fail(path, "must be a lowercase ASCII stable identifier")
    return text


def require_sha256(value: Any, path: str) -> str:
    text = require_text(value, path)
    if SHA256_PATTERN.fullmatch(text) is None:
        fail(path, "must be a lowercase SHA-256 digest")
    return text


def require_constant(value: Any, expected: Any, path: str) -> None:
    if type(value) is not type(expected) or value != expected:
        fail(path, f"must equal {expected!r}")


def validate_ordered_catalog(
    values: Any,
    path: str,
    ordinal_start: int,
    require_identifiers: bool = True,
) -> list[dict[str, Any]]:
    records = require_array(values, path)
    if not records:
        fail(path, "must not be empty")
    identifiers: set[str] = set()
    for index, raw in enumerate(records):
        record = require_object(raw, f"{path}[{index}]")
        if require_identifiers:
            identifier = require_id(record.get("id"), f"{path}[{index}].id")
            if identifier in identifiers:
                fail(path, f"duplicate id '{identifier}'")
            identifiers.add(identifier)
        ordinal = require_int(record.get("ordinal"), f"{path}[{index}].ordinal")
        expected = index + ordinal_start
        if ordinal != expected:
            fail(path, f"ordinals must be contiguous and ordered; expected {expected}")
    return records


def validate_tile(value: Any, width: int, height: int, path: str) -> tuple[int, int]:
    items = require_array(value, path)
    if len(items) != 2:
        fail(path, "must contain exactly [x, y]")
    x = require_int(items[0], f"{path}[0]", 0, width - 1)
    y = require_int(items[1], f"{path}[1]", 0, height - 1)
    return x, y


def validate_rectangle(
    value: Any,
    width: int,
    height: int,
    path: str,
) -> tuple[int, int, int, int]:
    record = require_object(value, path)
    require_exact_keys(record, {"min_x", "max_x", "min_y", "max_y"}, path)
    min_x = require_int(record["min_x"], f"{path}.min_x", 0, width - 1)
    max_x = require_int(record["max_x"], f"{path}.max_x", 0, width - 1)
    min_y = require_int(record["min_y"], f"{path}.min_y", 0, height - 1)
    max_y = require_int(record["max_y"], f"{path}.max_y", 0, height - 1)
    if min_x > max_x or min_y > max_y:
        fail(path, "rectangle minimum must not exceed maximum")
    return min_x, max_x, min_y, max_y


def rectangle_indices(
    rectangle: tuple[int, int, int, int], width: int
) -> list[int]:
    min_x, max_x, min_y, max_y = rectangle
    return [
        y * width + x
        for y in range(min_y, max_y + 1)
        for x in range(min_x, max_x + 1)
    ]


def tile_index(tile: tuple[int, int], width: int) -> int:
    return tile[1] * width + tile[0]


def index_tile(index: int, width: int) -> tuple[int, int]:
    return index % width, index // width


def neighbor_indices(index: int, width: int, height: int) -> Iterable[int]:
    x, y = index_tile(index, width)
    if y > 0:
        yield index - width
    if x + 1 < width:
        yield index + 1
    if y + 1 < height:
        yield index + width
    if x > 0:
        yield index - 1


def safe_project_path(project_root: Path, relative: str, path: str) -> Path:
    pure = PurePosixPath(relative)
    if pure.is_absolute() or ".." in pure.parts or not pure.parts:
        fail(path, "must be a project-relative path without traversal")
    root = project_root.resolve()
    candidate = (root / Path(*pure.parts)).resolve()
    try:
        candidate.relative_to(root)
    except ValueError:
        fail(path, "resolves outside the project root")
    return candidate


def load_base_descriptor(
    value: Any,
    project_root: Path,
) -> tuple[dict[str, Any], str, Path]:
    path = "base_descriptor"
    record = require_object(value, path)
    require_exact_keys(record, {"path", "canonical_sha256"}, path)
    relative = require_text(record["path"], f"{path}.path")
    declared = require_sha256(record["canonical_sha256"], f"{path}.canonical_sha256")
    require_constant(relative, BASE_DESCRIPTOR_PATH, f"{path}.path")
    require_constant(
        declared,
        BASE_DESCRIPTOR_CANONICAL_SHA256,
        f"{path}.canonical_sha256",
    )
    descriptor_path = safe_project_path(project_root, relative, f"{path}.path")
    descriptor = require_object(strict_json_load(descriptor_path), relative)
    actual = hashlib.sha256(canonical_bytes(descriptor)).hexdigest()
    if actual != declared:
        fail(path, f"canonical digest mismatch: declared {declared}, actual {actual}")
    sidecar_path = descriptor_path.with_suffix(".sha256")
    try:
        sidecar = sidecar_path.read_text(encoding="ascii").strip()
    except (OSError, UnicodeError) as error:
        fail(path, f"cannot read v1 digest sidecar: {error}")
    if sidecar != declared:
        fail(path, "v1 digest sidecar does not match the declared canonical digest")
    return descriptor, declared, descriptor_path


def base_context(descriptor: dict[str, Any]) -> dict[str, Any]:
    if descriptor.get("map_id") != "glass-scar":
        fail("base_descriptor.map_id", "must identify Glass Scar")
    grid = require_object(descriptor.get("grid"), "base_descriptor.grid")
    width = require_int(grid.get("width_tiles"), "base_descriptor.grid.width_tiles")
    height = require_int(grid.get("height_tiles"), "base_descriptor.grid.height_tiles")
    if width != 64 or height != 64:
        fail("base_descriptor.grid", "must retain the accepted 64x64 topology")

    terrain = require_object(descriptor.get("terrain"), "base_descriptor.terrain")
    scar = validate_rectangle(
        terrain.get("glass_scar_bounds"), width, height, "base_descriptor.terrain.glass_scar_bounds"
    )
    crossings: set[int] = set()
    for index, raw in enumerate(require_array(descriptor.get("routes"), "base_descriptor.routes")):
        route = require_object(raw, f"base_descriptor.routes[{index}]")
        rectangle = validate_rectangle(
            route.get("crossing"), width, height, f"base_descriptor.routes[{index}].crossing"
        )
        crossings.update(rectangle_indices(rectangle, width))
    blocked = set(rectangle_indices(scar, width)) - crossings
    if len(blocked) != 165:
        fail("base_descriptor.terrain", "accepted Glass Scar must contain 165 blocked cells")

    deployment = require_object(descriptor.get("deployment"), "base_descriptor.deployment")
    resources = require_object(descriptor.get("resources"), "base_descriptor.resources")
    objectives = require_object(descriptor.get("objectives"), "base_descriptor.objectives")
    checkpoints = require_object(
        descriptor.get("measurement_checkpoints"), "base_descriptor.measurement_checkpoints"
    )

    def tile_set(values: Any, path: str) -> set[int]:
        return {
            tile_index(validate_tile(value, width, height, f"{path}[{index}]"), width)
            for index, value in enumerate(require_array(values, path))
        }

    reserved = tile_set(deployment.get("local_spawns"), "base_descriptor.deployment.local_spawns")
    reserved.update(
        tile_set(deployment.get("opponent_spawns"), "base_descriptor.deployment.opponent_spawns")
    )
    reserved.update(
        tile_set(resources.get("matter_deposits"), "base_descriptor.resources.matter_deposits")
    )
    future_well = validate_tile(
        objectives.get("future_well"), width, height, "base_descriptor.objectives.future_well"
    )
    local_core = validate_tile(
        checkpoints.get("local_core"), width, height, "base_descriptor.measurement_checkpoints.local_core"
    )
    opponent_core = validate_tile(
        checkpoints.get("opponent_core"), width, height, "base_descriptor.measurement_checkpoints.opponent_core"
    )
    return {
        "width": width,
        "height": height,
        "blocked": blocked,
        "reserved": reserved,
        "future_well": future_well,
        "local_core": local_core,
        "opponent_core": opponent_core,
    }


def movement_mask_for_ids(
    values: Any,
    movement_by_id: dict[str, dict[str, Any]],
    path: str,
) -> int:
    identifiers = require_array(values, path)
    if not identifiers:
        fail(path, "must contain at least one movement class")
    seen: set[str] = set()
    ordinals: list[int] = []
    mask = 0
    for index, raw in enumerate(identifiers):
        identifier = require_id(raw, f"{path}[{index}]")
        if identifier in seen:
            fail(path, f"duplicate movement class '{identifier}'")
        seen.add(identifier)
        movement = movement_by_id.get(identifier)
        if movement is None:
            fail(path, f"unknown movement class '{identifier}'")
        ordinals.append(movement["ordinal"])
        mask |= movement["mask"]
    if ordinals != sorted(ordinals):
        fail(path, "movement classes must follow catalog ordinal order")
    return mask


def flood_indices(
    start: int,
    width: int,
    height: int,
    traversable: Callable[[int], bool],
) -> set[int]:
    visited = {start}
    frontier = deque([start])
    while frontier:
        current = frontier.popleft()
        for neighbor in neighbor_indices(current, width, height):
            if neighbor not in visited and traversable(neighbor):
                visited.add(neighbor)
                frontier.append(neighbor)
    return visited


def shortest_cost(
    pack: dict[str, Any],
    start_index: int,
    goal_index: int,
    movement_class_id: str = "ground",
    allowed_region_ordinals: set[int] | None = None,
) -> int | None:
    grid = pack["grid"]
    width = grid["width_tiles"]
    height = grid["height_tiles"]
    cell_count = grid["cell_count"]
    if not (0 <= start_index < cell_count and 0 <= goal_index < cell_count):
        return None
    movement = next(
        (item for item in pack["movement_classes"] if item["id"] == movement_class_id),
        None,
    )
    if movement is None:
        return None
    mask = movement["mask"]
    cell_masks = pack["cells"]["movement_mask"]
    costs = pack["cells"]["base_move_cost"]
    regions = pack["cells"]["region_ordinal"]

    def allowed(index: int) -> bool:
        if cell_masks[index] & mask == 0:
            return False
        return allowed_region_ordinals is None or regions[index] in allowed_region_ordinals

    if not allowed(start_index) or not allowed(goal_index):
        return None
    distances = {start_index: 0}
    frontier: list[tuple[int, int]] = [(0, start_index)]
    while frontier:
        distance, current = heapq.heappop(frontier)
        if distance != distances.get(current):
            continue
        if current == goal_index:
            return distance
        for neighbor in neighbor_indices(current, width, height):
            if not allowed(neighbor):
                continue
            candidate = distance + costs[neighbor]
            if candidate < distances.get(neighbor, 1 << 62):
                distances[neighbor] = candidate
                heapq.heappush(frontier, (candidate, neighbor))
    return None


def validate_compiled_pack(pack: Any) -> dict[str, Any]:
    root = require_object(pack, "compiled")
    require_exact_keys(root, COMPILED_KEYS, "compiled")
    require_constant(root["pack_format"], "echoes-compiled-map-pack", "compiled.pack_format")
    require_constant(root["pack_version"], 1, "compiled.pack_version")
    require_constant(root["schema_version"], 1, "compiled.schema_version")
    require_constant(root["map_id"], "glass-scar", "compiled.map_id")
    require_constant(root["display_name"], "Glass Scar", "compiled.display_name")
    require_constant(root["authority"], "checked-in-source-fixture", "compiled.authority")
    require_constant(root["runtime_binding"], "none", "compiled.runtime_binding")

    source_contract = require_object(root["source_contract"], "compiled.source_contract")
    require_exact_keys(
        source_contract,
        {
            "schema_id",
            "schema_version",
            "authoring_canonical_sha256",
            "base_descriptor_path",
            "base_descriptor_canonical_sha256",
        },
        "compiled.source_contract",
    )
    require_constant(
        source_contract["schema_id"], "echoes.world.map-source.v2", "compiled.source_contract.schema_id"
    )
    require_constant(source_contract["schema_version"], 2, "compiled.source_contract.schema_version")
    require_sha256(
        source_contract["authoring_canonical_sha256"],
        "compiled.source_contract.authoring_canonical_sha256",
    )
    require_constant(
        source_contract["base_descriptor_path"],
        BASE_DESCRIPTOR_PATH,
        "compiled.source_contract.base_descriptor_path",
    )
    require_constant(
        source_contract["base_descriptor_canonical_sha256"],
        BASE_DESCRIPTOR_CANONICAL_SHA256,
        "compiled.source_contract.base_descriptor_canonical_sha256",
    )

    canonicalization = require_object(root["canonicalization"], "compiled.canonicalization")
    require_exact_keys(canonicalization, set(CANONICALIZATION), "compiled.canonicalization")
    if canonicalization != CANONICALIZATION:
        fail("compiled.canonicalization", "does not match the compiled canonical profile")

    grid = require_object(root["grid"], "compiled.grid")
    require_exact_keys(grid, set(GRID_CONTRACT) | {"cell_count"}, "compiled.grid")
    for key, expected in GRID_CONTRACT.items():
        require_constant(grid[key], expected, f"compiled.grid.{key}")
    width = require_int(grid["width_tiles"], "compiled.grid.width_tiles", 1, 256)
    height = require_int(grid["height_tiles"], "compiled.grid.height_tiles", 1, 256)
    cell_count = require_int(grid["cell_count"], "compiled.grid.cell_count", 1, 65536)
    if cell_count != width * height:
        fail("compiled.grid.cell_count", "must equal width_tiles * height_tiles")

    movement_records = validate_ordered_catalog(
        root["movement_classes"], "compiled.movement_classes", 0
    )
    movement_by_id: dict[str, dict[str, Any]] = {}
    bits: set[int] = set()
    allowed_mask = 0
    for index, record in enumerate(movement_records):
        path = f"compiled.movement_classes[{index}]"
        require_exact_keys(record, {"id", "ordinal", "bit", "mask"}, path)
        bit = require_int(record["bit"], f"{path}.bit", 0, 30)
        if bit in bits:
            fail(path, f"duplicate movement bit {bit}")
        bits.add(bit)
        mask = require_int(record["mask"], f"{path}.mask", 1, 1 << 30)
        if mask != 1 << bit:
            fail(f"{path}.mask", "must equal 1 << bit")
        allowed_mask |= mask
        movement_by_id[record["id"]] = record
    ground = movement_by_id.get("ground")
    if (
        len(movement_records) != 1
        or ground is None
        or ground["ordinal"] != 0
        or ground["bit"] != 0
        or ground["mask"] != 1
    ):
        fail("compiled.movement_classes", "requires exactly ground at ordinal 0 and bit 0")

    height_records = validate_ordered_catalog(
        root["height_bands"], "compiled.height_bands", 0
    )
    height_ordinals: set[int] = set()
    for index, record in enumerate(height_records):
        path = f"compiled.height_bands[{index}]"
        require_exact_keys(record, {"id", "ordinal", "relative_level"}, path)
        require_int(record["relative_level"], f"{path}.relative_level", -8, 7)
        height_ordinals.add(record["ordinal"])

    region_records = validate_ordered_catalog(root["regions"], "compiled.regions", 1)
    if [item["id"] for item in region_records] != REQUIRED_REGION_IDS:
        fail("compiled.regions", "must retain the seven stable Glass Scar region identities")
    region_ordinals = {item["ordinal"] for item in region_records}
    region_by_id = {item["id"]: item for item in region_records}
    region_by_ordinal = {item["ordinal"]: item for item in region_records}
    for index, record in enumerate(region_records):
        path = f"compiled.regions[{index}]"
        require_exact_keys(
            record,
            {
                "id",
                "ordinal",
                "seed_index",
                "cell_count",
                "movement_mask",
                "base_move_cost",
                "height_band_ordinal",
            },
            path,
        )
        seed_index = require_int(record["seed_index"], f"{path}.seed_index", 0, cell_count - 1)
        expected_rectangle, expected_seed_tile = REQUIRED_REGION_GEOMETRY[record["id"]]
        if seed_index != tile_index(expected_seed_tile, width):
            fail(f"{path}.seed_index", "does not match the stable region seed")
        expected_count = len(rectangle_indices(expected_rectangle, width))
        cell_count_value = require_int(record["cell_count"], f"{path}.cell_count", 1, cell_count)
        if cell_count_value != expected_count:
            fail(f"{path}.cell_count", f"must equal the stable region size {expected_count}")
        mask = require_int(record["movement_mask"], f"{path}.movement_mask", 1, allowed_mask)
        if mask & ~allowed_mask:
            fail(f"{path}.movement_mask", "contains an undeclared movement bit")
        require_int(record["base_move_cost"], f"{path}.base_move_cost", 1, 255)
        band = require_int(record["height_band_ordinal"], f"{path}.height_band_ordinal")
        if band not in height_ordinals:
            fail(f"{path}.height_band_ordinal", "references an unknown height band")

    blocked_records = validate_ordered_catalog(
        root["blocked_zones"], "compiled.blocked_zones", 0
    )
    if [item["id"] for item in blocked_records] != list(REQUIRED_BLOCKED_GEOMETRY):
        fail("compiled.blocked_zones", "must retain the four stable blocked-zone identities")
    for index, record in enumerate(blocked_records):
        path = f"compiled.blocked_zones[{index}]"
        require_exact_keys(record, {"id", "ordinal", "cell_count", "height_band_ordinal"}, path)
        _, expected_count = REQUIRED_BLOCKED_GEOMETRY[record["id"]]
        actual_count = require_int(record["cell_count"], f"{path}.cell_count", 1, cell_count)
        if actual_count != expected_count:
            fail(f"{path}.cell_count", f"must equal the stable blocked-zone size {expected_count}")
        band = require_int(record["height_band_ordinal"], f"{path}.height_band_ordinal")
        if band not in height_ordinals:
            fail(f"{path}.height_band_ordinal", "references an unknown height band")

    cells = require_object(root["cells"], "compiled.cells")
    require_exact_keys(
        cells,
        {"movement_mask", "base_move_cost", "height_band_ordinal", "region_ordinal"},
        "compiled.cells",
    )
    arrays: dict[str, list[Any]] = {}
    for name in ("movement_mask", "base_move_cost", "height_band_ordinal", "region_ordinal"):
        array = require_array(cells[name], f"compiled.cells.{name}")
        if len(array) != cell_count:
            fail(f"compiled.cells.{name}", f"must contain exactly {cell_count} entries")
        arrays[name] = array

    actual_region_counts = {ordinal: 0 for ordinal in region_ordinals}
    expected_region_ordinals = [0] * cell_count
    for record in region_records:
        rectangle, _ = REQUIRED_REGION_GEOMETRY[record["id"]]
        for cell in rectangle_indices(rectangle, width):
            expected_region_ordinals[cell] = record["ordinal"]
    expected_blocked_heights = [-1] * cell_count
    for record in blocked_records:
        rectangle, _ = REQUIRED_BLOCKED_GEOMETRY[record["id"]]
        for cell in rectangle_indices(rectangle, width):
            expected_blocked_heights[cell] = record["height_band_ordinal"]

    blocked_count = 0
    for index in range(cell_count):
        mask = require_int(arrays["movement_mask"][index], f"compiled.cells.movement_mask[{index}]", 0)
        if mask & ~allowed_mask:
            fail(f"compiled.cells.movement_mask[{index}]", "contains an undeclared movement bit")
        cost = require_int(arrays["base_move_cost"][index], f"compiled.cells.base_move_cost[{index}]", 0, 255)
        band = require_int(arrays["height_band_ordinal"][index], f"compiled.cells.height_band_ordinal[{index}]")
        if band not in height_ordinals:
            fail(f"compiled.cells.height_band_ordinal[{index}]", "references an unknown height band")
        region = require_int(arrays["region_ordinal"][index], f"compiled.cells.region_ordinal[{index}]", 0)
        if region != expected_region_ordinals[index]:
            fail(
                f"compiled.cells.region_ordinal[{index}]",
                "does not match the stable region geometry",
            )
        if mask == 0:
            if cost != 0 or region != 0:
                fail(f"compiled.cells[{index}]", "blocked cells require zero mask, zero cost, and region sentinel 0")
            if band != expected_blocked_heights[index]:
                fail(
                    f"compiled.cells.height_band_ordinal[{index}]",
                    "does not match its blocked-zone catalog record",
                )
            blocked_count += 1
        else:
            if cost == 0 or region not in region_ordinals:
                fail(f"compiled.cells[{index}]", "passable cells require positive cost and a known nonzero region")
            region_record = region_by_ordinal[region]
            if (
                mask != region_record["movement_mask"]
                or cost != region_record["base_move_cost"]
                or band != region_record["height_band_ordinal"]
            ):
                fail(
                    f"compiled.cells[{index}]",
                    "movement mask, cost, and height must match its region catalog record",
                )
            actual_region_counts[region] += 1
    if blocked_count != 165:
        fail("compiled.cells", f"expected 165 blocked cells, found {blocked_count}")
    if cell_count - blocked_count != 3931:
        fail("compiled.cells", "expected 3,931 passable region cells")

    for record in region_records:
        ordinal = record["ordinal"]
        expected_count = REQUIRED_REGION_COUNTS[record["id"]]
        if record["cell_count"] != expected_count or actual_region_counts[ordinal] != expected_count:
            fail(f"compiled.regions.{record['id']}", f"must contain exactly {expected_count} cells")
        seed = record["seed_index"]
        if arrays["region_ordinal"][seed] != ordinal:
            fail(f"compiled.regions.{record['id']}.seed_index", "must lie in its region")

        def in_expected_region(candidate: int, expected: int = ordinal) -> bool:
            return arrays["region_ordinal"][candidate] == expected

        visited = flood_indices(
            seed,
            width,
            height,
            in_expected_region,
        )
        if len(visited) != expected_count:
            fail(f"compiled.regions.{record['id']}", "region cells are not cardinally connected")

    portal_records = validate_ordered_catalog(root["portals"], "compiled.portals", 0)
    if [item["id"] for item in portal_records] != REQUIRED_PORTAL_IDS:
        fail("compiled.portals", "must retain the ten stable Glass Scar portal identities")
    declared_edges: set[frozenset[int]] = set()
    declared_region_pairs: set[frozenset[int]] = set()
    for index, record in enumerate(portal_records):
        path = f"compiled.portals[{index}]"
        require_exact_keys(record, {"id", "ordinal", "region_ordinals", "edge_index_pairs"}, path)
        pair_regions = require_array(record["region_ordinals"], f"{path}.region_ordinals")
        if len(pair_regions) != 2:
            fail(f"{path}.region_ordinals", "must contain exactly two regions")
        first_region = require_int(pair_regions[0], f"{path}.region_ordinals[0]")
        second_region = require_int(pair_regions[1], f"{path}.region_ordinals[1]")
        expected_region_ids = REQUIRED_PORTAL_REGION_IDS[record["id"]]
        expected_regions = (
            region_by_id[expected_region_ids[0]]["ordinal"],
            region_by_id[expected_region_ids[1]]["ordinal"],
        )
        if (first_region, second_region) != expected_regions:
            fail(f"{path}.region_ordinals", "does not match the stable portal identity")
        if first_region not in region_ordinals or second_region not in region_ordinals:
            fail(f"{path}.region_ordinals", "references an unknown region")
        if first_region == second_region:
            fail(f"{path}.region_ordinals", "portal regions must be distinct")
        region_pair = frozenset((first_region, second_region))
        if region_pair in declared_region_pairs:
            fail(path, "duplicate portal region pair")
        declared_region_pairs.add(region_pair)
        edge_pairs = require_array(record["edge_index_pairs"], f"{path}.edge_index_pairs")
        if not edge_pairs:
            fail(f"{path}.edge_index_pairs", "must not be empty")
        prior: tuple[int, int] | None = None
        for edge_index, raw_pair in enumerate(edge_pairs):
            pair_path = f"{path}.edge_index_pairs[{edge_index}]"
            pair = require_array(raw_pair, pair_path)
            if len(pair) != 2:
                fail(pair_path, "must contain exactly two cell indices")
            first = require_int(pair[0], f"{pair_path}[0]", 0, cell_count - 1)
            second = require_int(pair[1], f"{pair_path}[1]", 0, cell_count - 1)
            first_xy = index_tile(first, width)
            second_xy = index_tile(second, width)
            if abs(first_xy[0] - second_xy[0]) + abs(first_xy[1] - second_xy[1]) != 1:
                fail(pair_path, "portal endpoints must be cardinally adjacent")
            if arrays["region_ordinal"][first] != first_region or arrays["region_ordinal"][second] != second_region:
                fail(pair_path, "portal endpoint regions do not match region_ordinals")
            edge = frozenset((first, second))
            if edge in declared_edges:
                fail(pair_path, "duplicate portal edge")
            declared_edges.add(edge)
            ordered_pair = (first, second)
            if prior is not None and ordered_pair <= prior:
                fail(f"{path}.edge_index_pairs", "must be strictly ordered by cell indices")
            prior = ordered_pair

    actual_edges: set[frozenset[int]] = set()
    for index in range(cell_count):
        region = arrays["region_ordinal"][index]
        if region == 0:
            continue
        x, y = index_tile(index, width)
        for neighbor in (
            index + 1 if x + 1 < width else None,
            index + width if y + 1 < height else None,
        ):
            if neighbor is None:
                continue
            other_region = arrays["region_ordinal"][neighbor]
            if other_region != 0 and other_region != region:
                actual_edges.add(frozenset((index, neighbor)))
    if declared_edges != actual_edges:
        missing = len(actual_edges - declared_edges)
        extra = len(declared_edges - actual_edges)
        fail("compiled.portals", f"portal edges do not match region boundaries (missing={missing}, extra={extra})")
    if len(declared_edges) != 62:
        fail("compiled.portals", f"expected 62 boundary edges, found {len(declared_edges)}")

    objective_records = validate_ordered_catalog(root["objectives"], "compiled.objectives", 0)
    if len(objective_records) != 1 or objective_records[0]["id"] != REQUIRED_OBJECTIVE_ID:
        fail("compiled.objectives", "must retain the single stable Future Well identity")
    objective_indices: set[int] = set()
    region_seeds = [record["seed_index"] for record in region_records]
    for index, record in enumerate(objective_records):
        path = f"compiled.objectives[{index}]"
        require_exact_keys(
            record,
            {
                "id",
                "ordinal",
                "type",
                "primary_index",
                "primary_region_ordinal",
                "fallback_policy",
                "fallbacks",
            },
            path,
        )
        require_constant(record["type"], "future-well", f"{path}.type")
        require_constant(record["fallback_policy"], "first-passable-in-order", f"{path}.fallback_policy")
        primary = require_int(record["primary_index"], f"{path}.primary_index", 0, cell_count - 1)
        if primary != tile_index(REQUIRED_FUTURE_WELL_TILE, width):
            fail(f"{path}.primary_index", "must equal the accepted v1 Future Well")
        primary_region = require_int(record["primary_region_ordinal"], f"{path}.primary_region_ordinal")
        if arrays["movement_mask"][primary] & ground["mask"] == 0:
            fail(f"{path}.primary_index", "objective primary must be ground-passable")
        if arrays["region_ordinal"][primary] != primary_region or primary_region not in region_ordinals:
            fail(f"{path}.primary_region_ordinal", "does not match the primary cell region")
        if primary in objective_indices:
            fail(path, "objective cells must be unique")
        objective_indices.add(primary)
        fallbacks = validate_ordered_catalog(
            record["fallbacks"], f"{path}.fallbacks", 0, require_identifiers=False
        )
        for fallback_index, fallback in enumerate(fallbacks):
            fallback_path = f"{path}.fallbacks[{fallback_index}]"
            require_exact_keys(fallback, {"ordinal", "index", "region_ordinal"}, fallback_path)
            cell = require_int(fallback["index"], f"{fallback_path}.index", 0, cell_count - 1)
            region = require_int(fallback["region_ordinal"], f"{fallback_path}.region_ordinal")
            if cell in objective_indices:
                fail(fallback_path, "objective primary and fallback cells must be unique")
            objective_indices.add(cell)
            if arrays["movement_mask"][cell] & ground["mask"] == 0:
                fail(f"{fallback_path}.index", "fallback must be ground-passable")
            if arrays["region_ordinal"][cell] != region or region not in region_ordinals:
                fail(f"{fallback_path}.region_ordinal", "does not match the fallback cell region")
        for seed in region_seeds:
            if shortest_cost(root, seed, primary, "ground") is None:
                fail(path, "objective primary is unreachable from a region seed")
            for fallback in fallbacks:
                if shortest_cost(root, seed, fallback["index"], "ground") is None:
                    fail(path, "objective fallback is unreachable from a region seed")

    bounds = require_object(root["camera_bounds"], "compiled.camera_bounds")
    require_exact_keys(
        bounds,
        {"coordinate_semantics", "min_x", "min_y", "max_x_exclusive", "max_y_exclusive"},
        "compiled.camera_bounds",
    )
    require_constant(
        bounds["coordinate_semantics"],
        "half-open-tile-edges",
        "compiled.camera_bounds.coordinate_semantics",
    )
    min_x = require_int(bounds["min_x"], "compiled.camera_bounds.min_x", 0, width - 1)
    min_y = require_int(bounds["min_y"], "compiled.camera_bounds.min_y", 0, height - 1)
    max_x = require_int(bounds["max_x_exclusive"], "compiled.camera_bounds.max_x_exclusive", 1, width)
    max_y = require_int(bounds["max_y_exclusive"], "compiled.camera_bounds.max_y_exclusive", 1, height)
    if min_x >= max_x or min_y >= max_y:
        fail("compiled.camera_bounds", "half-open minimum must be less than maximum")
    for index in range(cell_count):
        if arrays["region_ordinal"][index] == 0:
            continue
        x, y = index_tile(index, width)
        if not (min_x <= x < max_x and min_y <= y < max_y):
            fail("compiled.camera_bounds", "must contain every passable source-contract cell")

    claim = require_object(root["claim_boundary"], "compiled.claim_boundary")
    require_exact_keys(
        claim,
        {"proposed_contract", "compiled_fixture_is_source_data", "not_evidence_for"},
        "compiled.claim_boundary",
    )
    require_constant(claim["proposed_contract"], True, "compiled.claim_boundary.proposed_contract")
    require_constant(
        claim["compiled_fixture_is_source_data"],
        True,
        "compiled.claim_boundary.compiled_fixture_is_source_data",
    )
    exclusions = require_array(claim["not_evidence_for"], "compiled.claim_boundary.not_evidence_for")
    if exclusions != CLAIM_EXCLUSIONS:
        fail("compiled.claim_boundary.not_evidence_for", "must retain the ordered claim exclusions")

    south_ordinal = region_by_id["south-basin"]["ordinal"]
    north_ordinal = region_by_id["north-basin"]["ordinal"]
    for corridor_id, (south_tile, north_tile) in CORRIDOR_ANCHORS.items():
        corridor_ordinal = region_by_id[corridor_id]["ordinal"]
        corridor_cost = shortest_cost(
            root,
            tile_index(south_tile, width),
            tile_index(north_tile, width),
            "ground",
            {south_ordinal, north_ordinal, corridor_ordinal},
        )
        if corridor_cost is None:
            fail(f"compiled.regions.{corridor_id}", "corridor anchors are not ground-reachable")
    return root


def build_pack(source_path: Path, project_root: Path = PROJECT_ROOT) -> dict[str, Any]:
    source = require_object(strict_json_load(source_path), "source")
    require_exact_keys(source, SOURCE_KEYS, "source")
    require_constant(source["schema_id"], "echoes.world.map-source.v2", "source.schema_id")
    require_constant(source["schema_version"], 2, "source.schema_version")
    require_constant(source["map_id"], "glass-scar", "source.map_id")
    require_constant(source["display_name"], "Glass Scar", "source.display_name")
    require_constant(source["authority"], "proposed-source-contract", "source.authority")
    require_constant(source["runtime_binding"], "none", "source.runtime_binding")

    canonicalization = require_object(source["canonicalization"], "source.canonicalization")
    require_exact_keys(canonicalization, set(SOURCE_CANONICALIZATION), "source.canonicalization")
    if canonicalization != SOURCE_CANONICALIZATION:
        fail("source.canonicalization", "does not match the source canonical profile")

    grid = require_object(source["grid"], "source.grid")
    require_exact_keys(grid, set(GRID_CONTRACT), "source.grid")
    if grid != GRID_CONTRACT:
        fail("source.grid", "must retain the accepted Glass Scar grid and scale contract")
    width = GRID_CONTRACT["width_tiles"]
    height = GRID_CONTRACT["height_tiles"]
    cell_count = width * height

    base_descriptor, base_digest, _ = load_base_descriptor(source["base_descriptor"], project_root)
    base = base_context(base_descriptor)

    movement_source = validate_ordered_catalog(
        source["movement_classes"], "source.movement_classes", 0
    )
    movement_by_id: dict[str, dict[str, Any]] = {}
    bits: set[int] = set()
    compiled_movement: list[dict[str, Any]] = []
    for index, record in enumerate(movement_source):
        path = f"source.movement_classes[{index}]"
        require_exact_keys(record, {"id", "ordinal", "bit"}, path)
        bit = require_int(record["bit"], f"{path}.bit", 0, 30)
        if bit in bits:
            fail(path, f"duplicate movement bit {bit}")
        bits.add(bit)
        compiled = {"id": record["id"], "ordinal": record["ordinal"], "bit": bit, "mask": 1 << bit}
        movement_by_id[record["id"]] = compiled
        compiled_movement.append(compiled)
    if set(movement_by_id) != {"ground"} or movement_by_id["ground"]["bit"] != 0:
        fail("source.movement_classes", "this v2 contract requires exactly ground at bit 0")

    height_source = validate_ordered_catalog(source["height_bands"], "source.height_bands", 0)
    height_by_id: dict[str, dict[str, Any]] = {}
    compiled_heights: list[dict[str, Any]] = []
    for index, record in enumerate(height_source):
        path = f"source.height_bands[{index}]"
        require_exact_keys(record, {"id", "ordinal", "relative_level"}, path)
        relative = require_int(record["relative_level"], f"{path}.relative_level", -8, 7)
        compiled = {"id": record["id"], "ordinal": record["ordinal"], "relative_level": relative}
        height_by_id[record["id"]] = compiled
        compiled_heights.append(compiled)

    ownership: list[tuple[str, str] | None] = [None] * cell_count
    movement_masks = [0] * cell_count
    base_costs = [0] * cell_count
    height_ordinals = [-1] * cell_count
    region_ordinals = [0] * cell_count

    region_source = validate_ordered_catalog(source["regions"], "source.regions", 1)
    if [item["id"] for item in region_source] != REQUIRED_REGION_IDS:
        fail("source.regions", "must retain the seven stable Glass Scar region identities")
    region_by_id: dict[str, dict[str, Any]] = {}
    compiled_regions: list[dict[str, Any]] = []
    for index, record in enumerate(region_source):
        path = f"source.regions[{index}]"
        require_exact_keys(
            record,
            {
                "id",
                "ordinal",
                "rectangle",
                "seed_tile",
                "movement_classes",
                "base_move_cost",
                "height_band_id",
            },
            path,
        )
        rectangle = validate_rectangle(record["rectangle"], width, height, f"{path}.rectangle")
        expected_rectangle, expected_seed_tile = REQUIRED_REGION_GEOMETRY[record["id"]]
        if rectangle != expected_rectangle:
            fail(f"{path}.rectangle", "does not match the stable region geometry")
        indices = rectangle_indices(rectangle, width)
        seed_tile = validate_tile(record["seed_tile"], width, height, f"{path}.seed_tile")
        if seed_tile != expected_seed_tile:
            fail(f"{path}.seed_tile", "does not match the stable region seed")
        seed = tile_index(seed_tile, width)
        if seed not in indices:
            fail(f"{path}.seed_tile", "must lie inside the region rectangle")
        mask = movement_mask_for_ids(record["movement_classes"], movement_by_id, f"{path}.movement_classes")
        cost = require_int(record["base_move_cost"], f"{path}.base_move_cost", 1, 255)
        height_id = require_id(record["height_band_id"], f"{path}.height_band_id")
        height_record = height_by_id.get(height_id)
        if height_record is None:
            fail(f"{path}.height_band_id", f"unknown height band '{height_id}'")
        for cell in indices:
            prior_owner = ownership[cell]
            if prior_owner is not None:
                fail(f"{path}.rectangle", f"overlaps {prior_owner[0]} '{prior_owner[1]}'")
            ownership[cell] = ("region", record["id"])
            movement_masks[cell] = mask
            base_costs[cell] = cost
            height_ordinals[cell] = height_record["ordinal"]
            region_ordinals[cell] = record["ordinal"]
        compiled = {
            "id": record["id"],
            "ordinal": record["ordinal"],
            "seed_index": seed,
            "cell_count": len(indices),
            "movement_mask": mask,
            "base_move_cost": cost,
            "height_band_ordinal": height_record["ordinal"],
        }
        region_by_id[record["id"]] = compiled
        compiled_regions.append(compiled)

    blocked_source = validate_ordered_catalog(
        source["blocked_zones"], "source.blocked_zones", 0
    )
    if [item["id"] for item in blocked_source] != list(REQUIRED_BLOCKED_GEOMETRY):
        fail("source.blocked_zones", "must retain the four stable blocked-zone identities")
    compiled_blocked: list[dict[str, Any]] = []
    blocked_indices: set[int] = set()
    for index, record in enumerate(blocked_source):
        path = f"source.blocked_zones[{index}]"
        require_exact_keys(record, {"id", "ordinal", "rectangle", "height_band_id"}, path)
        rectangle = validate_rectangle(record["rectangle"], width, height, f"{path}.rectangle")
        expected_rectangle, expected_count = REQUIRED_BLOCKED_GEOMETRY[record["id"]]
        if rectangle != expected_rectangle:
            fail(f"{path}.rectangle", "does not match the stable blocked-zone geometry")
        indices = rectangle_indices(rectangle, width)
        if len(indices) != expected_count:
            fail(f"{path}.rectangle", f"must contain exactly {expected_count} cells")
        height_id = require_id(record["height_band_id"], f"{path}.height_band_id")
        height_record = height_by_id.get(height_id)
        if height_record is None:
            fail(f"{path}.height_band_id", f"unknown height band '{height_id}'")
        for cell in indices:
            prior_owner = ownership[cell]
            if prior_owner is not None:
                fail(f"{path}.rectangle", f"overlaps {prior_owner[0]} '{prior_owner[1]}'")
            ownership[cell] = ("blocked-zone", record["id"])
            height_ordinals[cell] = height_record["ordinal"]
            blocked_indices.add(cell)
        compiled_blocked.append(
            {
                "id": record["id"],
                "ordinal": record["ordinal"],
                "cell_count": len(indices),
                "height_band_ordinal": height_record["ordinal"],
            }
        )

    uncovered = [index for index, owner in enumerate(ownership) if owner is None]
    if uncovered:
        x, y = index_tile(uncovered[0], width)
        fail("source", f"region/blocked-zone coverage gap at tile [{x}, {y}]")
    if blocked_indices != base["blocked"]:
        missing = len(base["blocked"] - blocked_indices)
        extra = len(blocked_indices - base["blocked"])
        fail("source.blocked_zones", f"does not match accepted v1 passability (missing={missing}, extra={extra})")
    for record in compiled_regions:
        expected = REQUIRED_REGION_COUNTS[record["id"]]
        if record["cell_count"] != expected:
            fail(f"source.regions.{record['id']}", f"must contain exactly {expected} cells")
    if region_by_id["south-basin"]["seed_index"] != tile_index(base["local_core"], width):
        fail("source.regions.south-basin.seed_tile", "must equal the accepted local core checkpoint")
    if region_by_id["north-basin"]["seed_index"] != tile_index(base["opponent_core"], width):
        fail("source.regions.north-basin.seed_tile", "must equal the accepted opponent core checkpoint")

    portal_source = validate_ordered_catalog(source["portals"], "source.portals", 0)
    if [item["id"] for item in portal_source] != REQUIRED_PORTAL_IDS:
        fail("source.portals", "must retain the ten stable Glass Scar portal identities")
    compiled_portals: list[dict[str, Any]] = []
    declared_edges: set[frozenset[int]] = set()
    region_pairs: set[frozenset[int]] = set()
    for index, record in enumerate(portal_source):
        path = f"source.portals[{index}]"
        require_exact_keys(record, {"id", "ordinal", "region_ids", "edge_pairs"}, path)
        region_ids = require_array(record["region_ids"], f"{path}.region_ids")
        if len(region_ids) != 2:
            fail(f"{path}.region_ids", "must contain exactly two region IDs")
        first_id = require_id(region_ids[0], f"{path}.region_ids[0]")
        second_id = require_id(region_ids[1], f"{path}.region_ids[1]")
        if (first_id, second_id) != REQUIRED_PORTAL_REGION_IDS[record["id"]]:
            fail(f"{path}.region_ids", "does not match the stable portal identity")
        if first_id == second_id:
            fail(f"{path}.region_ids", "must identify two distinct regions")
        if first_id not in region_by_id or second_id not in region_by_id:
            fail(f"{path}.region_ids", "references an unknown region")
        first_ordinal = region_by_id[first_id]["ordinal"]
        second_ordinal = region_by_id[second_id]["ordinal"]
        region_pair = frozenset((first_ordinal, second_ordinal))
        if region_pair in region_pairs:
            fail(path, "duplicate portal region pair")
        region_pairs.add(region_pair)
        edges = require_array(record["edge_pairs"], f"{path}.edge_pairs")
        if not edges:
            fail(f"{path}.edge_pairs", "must not be empty")
        compiled_edges: list[list[int]] = []
        prior: tuple[int, int] | None = None
        for edge_index, raw in enumerate(edges):
            edge_path = f"{path}.edge_pairs[{edge_index}]"
            pair = require_array(raw, edge_path)
            if len(pair) != 2:
                fail(edge_path, "must contain exactly two endpoint tiles")
            first_tile = validate_tile(pair[0], width, height, f"{edge_path}[0]")
            second_tile = validate_tile(pair[1], width, height, f"{edge_path}[1]")
            if abs(first_tile[0] - second_tile[0]) + abs(first_tile[1] - second_tile[1]) != 1:
                fail(edge_path, "portal endpoint tiles must be cardinally adjacent")
            first = tile_index(first_tile, width)
            second = tile_index(second_tile, width)
            if region_ordinals[first] != first_ordinal or region_ordinals[second] != second_ordinal:
                fail(edge_path, "portal endpoint tiles do not match region_ids")
            edge = frozenset((first, second))
            if edge in declared_edges:
                fail(edge_path, "duplicate portal edge")
            declared_edges.add(edge)
            ordered = (first, second)
            if prior is not None and ordered <= prior:
                fail(f"{path}.edge_pairs", "must be strictly ordered by row-major cell index")
            prior = ordered
            compiled_edges.append([first, second])
        compiled_portals.append(
            {
                "id": record["id"],
                "ordinal": record["ordinal"],
                "region_ordinals": [first_ordinal, second_ordinal],
                "edge_index_pairs": compiled_edges,
            }
        )

    actual_edges: set[frozenset[int]] = set()
    for cell in range(cell_count):
        region = region_ordinals[cell]
        if region == 0:
            continue
        x, y = index_tile(cell, width)
        for neighbor in (
            cell + 1 if x + 1 < width else None,
            cell + width if y + 1 < height else None,
        ):
            if neighbor is None:
                continue
            other = region_ordinals[neighbor]
            if other != 0 and other != region:
                actual_edges.add(frozenset((cell, neighbor)))
    if declared_edges != actual_edges:
        fail("source.portals", "must exactly declare every cross-region cardinal adjacency")
    if len(declared_edges) != 62:
        fail("source.portals", f"expected 62 boundary edges, found {len(declared_edges)}")

    claim = require_object(source["claim_boundary"], "source.claim_boundary")
    require_exact_keys(
        claim,
        {"proposed_contract", "compiled_fixture_is_source_data", "not_evidence_for"},
        "source.claim_boundary",
    )
    require_bool(claim["proposed_contract"], "source.claim_boundary.proposed_contract")
    require_bool(
        claim["compiled_fixture_is_source_data"],
        "source.claim_boundary.compiled_fixture_is_source_data",
    )
    if claim["proposed_contract"] is not True or claim["compiled_fixture_is_source_data"] is not True:
        fail("source.claim_boundary", "must retain proposed/source-fixture status")
    exclusions = require_array(claim["not_evidence_for"], "source.claim_boundary.not_evidence_for")
    if exclusions != CLAIM_EXCLUSIONS:
        fail("source.claim_boundary.not_evidence_for", "must retain the ordered claim exclusions")

    objective_source = validate_ordered_catalog(source["objectives"], "source.objectives", 0)
    if len(objective_source) != 1 or objective_source[0]["id"] != REQUIRED_OBJECTIVE_ID:
        fail("source.objectives", "must retain the single stable Future Well identity")
    compiled_objectives: list[dict[str, Any]] = []
    occupied: set[int] = set()
    for index, record in enumerate(objective_source):
        path = f"source.objectives[{index}]"
        require_exact_keys(
            record,
            {
                "id",
                "ordinal",
                "type",
                "primary_tile",
                "primary_region_id",
                "fallback_policy",
                "fallbacks",
            },
            path,
        )
        objective_type = require_id(record["type"], f"{path}.type")
        if objective_type != "future-well":
            fail(f"{path}.type", "unsupported objective type")
        require_constant(record["fallback_policy"], "first-passable-in-order", f"{path}.fallback_policy")
        primary_tile = validate_tile(record["primary_tile"], width, height, f"{path}.primary_tile")
        if primary_tile != REQUIRED_FUTURE_WELL_TILE or primary_tile != base["future_well"]:
            fail(f"{path}.primary_tile", "must equal the accepted v1 Future Well")
        primary = tile_index(primary_tile, width)
        primary_region_id = require_id(record["primary_region_id"], f"{path}.primary_region_id")
        primary_region = region_by_id.get(primary_region_id)
        if primary_region is None or region_ordinals[primary] != primary_region["ordinal"]:
            fail(f"{path}.primary_region_id", "does not match the primary tile region")
        if movement_masks[primary] & movement_by_id["ground"]["mask"] == 0:
            fail(f"{path}.primary_tile", "must be ground-passable")
        if primary in occupied or primary in base["reserved"]:
            fail(f"{path}.primary_tile", "conflicts with a fixed placement")
        occupied.add(primary)
        fallback_source = validate_ordered_catalog(
            record["fallbacks"], f"{path}.fallbacks", 0, require_identifiers=False
        )
        compiled_fallbacks: list[dict[str, Any]] = []
        for fallback_index, fallback in enumerate(fallback_source):
            fallback_path = f"{path}.fallbacks[{fallback_index}]"
            require_exact_keys(fallback, {"ordinal", "tile", "region_id"}, fallback_path)
            tile = validate_tile(fallback["tile"], width, height, f"{fallback_path}.tile")
            cell = tile_index(tile, width)
            region_id = require_id(fallback["region_id"], f"{fallback_path}.region_id")
            fallback_region = region_by_id.get(region_id)
            if (
                fallback_region is None
                or region_ordinals[cell] != fallback_region["ordinal"]
            ):
                fail(f"{fallback_path}.region_id", "does not match the fallback tile region")
            if movement_masks[cell] & movement_by_id["ground"]["mask"] == 0:
                fail(f"{fallback_path}.tile", "must be ground-passable")
            if cell in occupied:
                fail(fallback_path, "objective primary and fallbacks must be unique")
            if cell in base["reserved"]:
                fail(f"{fallback_path}.tile", "conflicts with a fixed deployment or resource")
            occupied.add(cell)
            compiled_fallbacks.append(
                {
                    "ordinal": fallback["ordinal"],
                    "index": cell,
                    "region_ordinal": fallback_region["ordinal"],
                }
            )
        compiled_objectives.append(
            {
                "id": record["id"],
                "ordinal": record["ordinal"],
                "type": objective_type,
                "primary_index": primary,
                "primary_region_ordinal": primary_region["ordinal"],
                "fallback_policy": "first-passable-in-order",
                "fallbacks": compiled_fallbacks,
            }
        )

    bounds = require_object(source["camera_bounds"], "source.camera_bounds")
    require_exact_keys(
        bounds,
        {"coordinate_semantics", "min_x", "min_y", "max_x_exclusive", "max_y_exclusive"},
        "source.camera_bounds",
    )
    require_constant(
        bounds["coordinate_semantics"], "half-open-tile-edges", "source.camera_bounds.coordinate_semantics"
    )
    min_x = require_int(bounds["min_x"], "source.camera_bounds.min_x", 0, width - 1)
    min_y = require_int(bounds["min_y"], "source.camera_bounds.min_y", 0, height - 1)
    max_x = require_int(bounds["max_x_exclusive"], "source.camera_bounds.max_x_exclusive", 1, width)
    max_y = require_int(bounds["max_y_exclusive"], "source.camera_bounds.max_y_exclusive", 1, height)
    if min_x >= max_x or min_y >= max_y:
        fail("source.camera_bounds", "half-open minimum must be less than maximum")
    for cell, owner in enumerate(ownership):
        if owner is None or owner[0] != "region":
            continue
        x, y = index_tile(cell, width)
        if not (min_x <= x < max_x and min_y <= y < max_y):
            fail("source.camera_bounds", "must contain every passable source-contract cell")

    authoring_digest = hashlib.sha256(canonical_bytes(source)).hexdigest()
    pack = {
        "pack_format": "echoes-compiled-map-pack",
        "pack_version": 1,
        "schema_version": 1,
        "map_id": "glass-scar",
        "display_name": "Glass Scar",
        "authority": "checked-in-source-fixture",
        "runtime_binding": "none",
        "source_contract": {
            "schema_id": "echoes.world.map-source.v2",
            "schema_version": 2,
            "authoring_canonical_sha256": authoring_digest,
            "base_descriptor_path": source["base_descriptor"]["path"],
            "base_descriptor_canonical_sha256": base_digest,
        },
        "canonicalization": dict(CANONICALIZATION),
        "grid": {**GRID_CONTRACT, "cell_count": cell_count},
        "movement_classes": compiled_movement,
        "height_bands": compiled_heights,
        "regions": compiled_regions,
        "blocked_zones": compiled_blocked,
        "portals": compiled_portals,
        "cells": {
            "movement_mask": movement_masks,
            "base_move_cost": base_costs,
            "height_band_ordinal": height_ordinals,
            "region_ordinal": region_ordinals,
        },
        "objectives": compiled_objectives,
        "camera_bounds": dict(bounds),
        "claim_boundary": {
            "proposed_contract": True,
            "compiled_fixture_is_source_data": True,
            "not_evidence_for": list(CLAIM_EXCLUSIONS),
        },
    }
    return validate_compiled_pack(pack)


def compile_pack(
    source_path: Path,
    output_path: Path,
    digest_output: Path | None = None,
    project_root: Path = PROJECT_ROOT,
) -> str:
    digest_path = digest_output or output_path.with_suffix(".sha256")
    if paths_collide(output_path, digest_path):
        fail("output", "compiled JSON and digest destinations must be distinct")

    base_path = project_root / BASE_DESCRIPTOR_PATH
    protected_inputs = {
        "authoring source": source_path,
        "accepted v1 descriptor": base_path,
        "accepted v1 digest": base_path.with_suffix(".sha256"),
    }
    for destination_name, destination in (
        ("compiled JSON destination", output_path),
        ("digest destination", digest_path),
    ):
        for protected_name, protected in protected_inputs.items():
            if paths_collide(destination, protected):
                fail(
                    "output",
                    f"{destination_name} must not overwrite the {protected_name}",
                )

    pack = build_pack(source_path, project_root)
    data = canonical_bytes(pack)
    digest = hashlib.sha256(data).hexdigest()
    transactional_write_pair(
        output_path,
        data,
        digest_path,
        f"{digest}\n".encode("ascii"),
    )
    return digest


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--digest-output", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        digest = compile_pack(arguments.source, arguments.output, arguments.digest_output)
    except MapContractError as error:
        print(f"Map contract validation failed: {error}", file=sys.stderr)
        return 2
    except OSError as error:
        print(f"Map compiler I/O failed: {error}", file=sys.stderr)
        return 3
    print(f"Compiled proposed map source: {arguments.output}")
    print(f"Compiled map SHA-256: {digest}")
    print("Runtime binding: none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
