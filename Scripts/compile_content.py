#!/usr/bin/env python3
"""Validate and canonically compile Echoes authoritative source data."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import tempfile
from pathlib import Path
from typing import Any, NoReturn


PACK_FORMAT = "echoes-content-pack"
PACK_VERSION = 1
SCHEMA_VERSION = 1
ID_PATTERN = re.compile(r"^[a-z][a-z0-9_]{2,63}$")
COLOR_PATTERN = re.compile(r"^#[0-9A-Fa-f]{6}$")
SOURCE_FILES = ("factions.json", "units.json", "buildings.json", "future_wells.json")


class ContentValidationError(ValueError):
    """Raised when authoritative content fails closed validation."""


def fail(path: str, message: str) -> NoReturn:
    raise ContentValidationError(f"{path}: {message}")


def require_object(value: Any, path: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        fail(path, "expected an object")
    return value


def require_array(value: Any, path: str) -> list[Any]:
    if not isinstance(value, list):
        fail(path, "expected an array")
    return value


def require_exact_keys(
    value: dict[str, Any],
    required: set[str],
    optional: set[str],
    path: str,
) -> None:
    missing = sorted(required - value.keys())
    unknown = sorted(value.keys() - required - optional)
    if missing:
        fail(path, f"missing required fields: {', '.join(missing)}")
    if unknown:
        fail(path, f"unknown fields: {', '.join(unknown)}")


def require_int(value: Any, path: str, minimum: int, maximum: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        fail(path, "expected an integer")
    if value < minimum or value > maximum:
        fail(path, f"must be between {minimum} and {maximum}")
    return value


def require_bool(value: Any, path: str) -> bool:
    if not isinstance(value, bool):
        fail(path, "expected a boolean")
    return value


def require_text(value: Any, path: str, minimum: int = 1, maximum: int = 80) -> str:
    if not isinstance(value, str):
        fail(path, "expected a string")
    if len(value) < minimum or len(value) > maximum or value.strip() != value:
        fail(path, f"must contain {minimum}..{maximum} trimmed characters")
    return value


def require_id(value: Any, path: str) -> str:
    text = require_text(value, path, 3, 64)
    if ID_PATTERN.fullmatch(text) is None:
        fail(path, "must match [a-z][a-z0-9_]{2,63}")
    return text


def require_schema(root: dict[str, Any], collection: str, path: str) -> list[Any]:
    require_exact_keys(root, {"schema_version", collection}, set(), path)
    if require_int(root["schema_version"], f"{path}.schema_version", 1, 1) != SCHEMA_VERSION:
        fail(f"{path}.schema_version", f"unsupported schema; expected {SCHEMA_VERSION}")
    values = require_array(root[collection], f"{path}.{collection}")
    if not values:
        fail(f"{path}.{collection}", "must not be empty")
    return values


def require_cost(value: Any, path: str) -> dict[str, int]:
    cost = require_object(value, path)
    require_exact_keys(cost, {"matter", "dawn"}, set(), path)
    return {
        "matter": require_int(cost["matter"], f"{path}.matter", 0, 1_000_000),
        "dawn": require_int(cost["dawn"], f"{path}.dawn", 0, 1_000_000),
    }


def load_json(source_dir: Path, filename: str) -> dict[str, Any]:
    path = source_dir / filename
    try:
        raw = path.read_text(encoding="utf-8")
    except OSError as error:
        fail(filename, f"cannot read source file: {error}")
    try:
        value = json.loads(raw)
    except json.JSONDecodeError as error:
        fail(filename, f"invalid JSON at line {error.lineno}, column {error.colno}")
    return require_object(value, filename)


def validate_factions(root: dict[str, Any]) -> tuple[list[dict[str, Any]], set[str]]:
    records = require_schema(root, "factions", "factions.json")
    output: list[dict[str, Any]] = []
    ids: set[str] = set()
    playable: set[str] = set()
    for index, raw in enumerate(records):
        path = f"factions.json.factions[{index}]"
        record = require_object(raw, path)
        require_exact_keys(
            record,
            {"id", "display_name", "identity", "primary_color", "secondary_color"},
            {"vertical_slice_playable"},
            path,
        )
        identifier = require_id(record["id"], f"{path}.id")
        if identifier in ids:
            fail(f"{path}.id", f"duplicate faction id '{identifier}'")
        ids.add(identifier)
        identities = require_array(record["identity"], f"{path}.identity")
        normalized_identities = [
            require_id(item, f"{path}.identity[{item_index}]")
            for item_index, item in enumerate(identities)
        ]
        if not normalized_identities or len(set(normalized_identities)) != len(normalized_identities):
            fail(f"{path}.identity", "must contain unique identity tags")
        colors: dict[str, str] = {}
        for field in ("primary_color", "secondary_color"):
            color = require_text(record[field], f"{path}.{field}", 7, 7).upper()
            if COLOR_PATTERN.fullmatch(color) is None:
                fail(f"{path}.{field}", "must be a six-digit hexadecimal color")
            colors[field] = color
        is_playable = record.get("vertical_slice_playable", True)
        require_bool(is_playable, f"{path}.vertical_slice_playable")
        if is_playable:
            playable.add(identifier)
        output.append(
            {
                "id": identifier,
                "display_name": require_text(record["display_name"], f"{path}.display_name"),
                "identity": sorted(normalized_identities),
                "vertical_slice_playable": is_playable,
                **colors,
            }
        )
    if len(playable) < 2:
        fail("factions.json.factions", "at least two vertical-slice factions must be playable")
    return sorted(output, key=lambda item: item["id"]), playable


def validate_units(
    root: dict[str, Any], faction_ids: set[str], playable: set[str]
) -> list[dict[str, Any]]:
    records = require_schema(root, "units", "units.json")
    output: list[dict[str, Any]] = []
    ids: set[str] = set()
    roles_by_faction: dict[str, set[str]] = {identifier: set() for identifier in playable}
    for index, raw in enumerate(records):
        path = f"units.json.units[{index}]"
        record = require_object(raw, path)
        require_exact_keys(
            record,
            {"id", "display_name", "faction", "role", "cost", "max_health", "move_speed_cm_s", "sight_cm", "population_cost", "production_ticks"},
            {"work_rate", "cargo_capacity", "attack", "deployment", "supply_extension"},
            path,
        )
        identifier = require_id(record["id"], f"{path}.id")
        if identifier in ids:
            fail(f"{path}.id", f"duplicate unit id '{identifier}'")
        ids.add(identifier)
        faction = require_id(record["faction"], f"{path}.faction")
        if faction not in faction_ids:
            fail(f"{path}.faction", f"unknown faction '{faction}'")
        if faction not in playable:
            fail(f"{path}.faction", "units for a non-playable slice faction are not accepted")
        role = require_id(record["role"], f"{path}.role")
        roles_by_faction[faction].add(role)
        cargo = require_int(record.get("cargo_capacity", 0), f"{path}.cargo_capacity", 0, 100_000)
        work_rate = require_int(record.get("work_rate", 0), f"{path}.work_rate", 0, 100_000)
        attack: dict[str, int] | None = None
        if "attack" in record:
            raw_attack = require_object(record["attack"], f"{path}.attack")
            require_exact_keys(raw_attack, {"damage", "range_cm", "cooldown_ticks"}, set(), f"{path}.attack")
            attack = {
                "damage": require_int(raw_attack["damage"], f"{path}.attack.damage", 1, 1_000_000),
                "range_cm": require_int(raw_attack["range_cm"], f"{path}.attack.range_cm", 0, 100_000),
                "cooldown_ticks": require_int(raw_attack["cooldown_ticks"], f"{path}.attack.cooldown_ticks", 1, 100_000),
            }
        if role == "worker" and (cargo == 0 or work_rate == 0):
            fail(f"{path}.cargo_capacity", "workers require positive cargo capacity and work rate")
        if role != "worker" and attack is None:
            fail(f"{path}.attack", "non-worker slice units require an attack definition")
        deployment: dict[str, int] | None = None
        if "deployment" in record:
            raw_deployment = require_object(record["deployment"], f"{path}.deployment")
            require_exact_keys(
                raw_deployment,
                {"cover_depth_cm", "cover_half_width_cm", "damage_reduction_percent", "move_speed_percent"},
                set(),
                f"{path}.deployment",
            )
            if faction != "meridian_compact" or role != "heavy_screen":
                fail(f"{path}.deployment", "schema 1 deployment is reserved for the Meridian heavy screen")
            deployment = {
                "cover_depth_cm": require_int(raw_deployment["cover_depth_cm"], f"{path}.deployment.cover_depth_cm", 100, 1600),
                "cover_half_width_cm": require_int(raw_deployment["cover_half_width_cm"], f"{path}.deployment.cover_half_width_cm", 100, 1600),
                "damage_reduction_percent": require_int(raw_deployment["damage_reduction_percent"], f"{path}.deployment.damage_reduction_percent", 1, 99),
                "move_speed_percent": require_int(raw_deployment["move_speed_percent"], f"{path}.deployment.move_speed_percent", 1, 99),
            }
        if faction == "meridian_compact" and role == "heavy_screen" and deployment is None:
            fail(f"{path}.deployment", "the Meridian heavy screen requires authored deployment rules")
        supply_extension: dict[str, int] | None = None
        if "supply_extension" in record:
            raw_supply = require_object(record["supply_extension"], f"{path}.supply_extension")
            require_exact_keys(
                raw_supply,
                {"connection_radius_cm", "capacity_bonus", "duration_ticks", "cooldown_ticks"},
                set(),
                f"{path}.supply_extension",
            )
            if faction != "meridian_compact" or role != "scout_support":
                fail(f"{path}.supply_extension", "schema 1 supply extension is reserved for the Meridian scout support")
            supply_extension = {
                "connection_radius_cm": require_int(raw_supply["connection_radius_cm"], f"{path}.supply_extension.connection_radius_cm", 100, 3200),
                "capacity_bonus": require_int(raw_supply["capacity_bonus"], f"{path}.supply_extension.capacity_bonus", 1, 1000),
                "duration_ticks": require_int(raw_supply["duration_ticks"], f"{path}.supply_extension.duration_ticks", 1, 1_000_000),
                "cooldown_ticks": require_int(raw_supply["cooldown_ticks"], f"{path}.supply_extension.cooldown_ticks", 1, 1_000_000),
            }
            if supply_extension["cooldown_ticks"] < supply_extension["duration_ticks"]:
                fail(f"{path}.supply_extension.cooldown_ticks", "must be at least duration_ticks")
        if faction == "meridian_compact" and role == "scout_support" and supply_extension is None:
            fail(f"{path}.supply_extension", "the Meridian scout support requires authored supply-extension rules")
        output.append(
            {
                "id": identifier,
                "display_name": require_text(record["display_name"], f"{path}.display_name"),
                "faction": faction,
                "role": role,
                "cost": require_cost(record["cost"], f"{path}.cost"),
                "max_health": require_int(record["max_health"], f"{path}.max_health", 1, 1_000_000),
                "move_speed_cm_s": require_int(record["move_speed_cm_s"], f"{path}.move_speed_cm_s", 1, 100_000),
                "sight_cm": require_int(record["sight_cm"], f"{path}.sight_cm", 100, 100_000),
                "population_cost": require_int(record["population_cost"], f"{path}.population_cost", 1, 1_000),
                "production_ticks": require_int(record["production_ticks"], f"{path}.production_ticks", 1, 1_000_000),
                "work_rate": work_rate,
                "cargo_capacity": cargo,
                "attack": attack,
                "deployment": deployment,
                "supply_extension": supply_extension,
            }
        )
    for faction in sorted(playable):
        roles = roles_by_faction[faction]
        if "worker" not in roles or len(roles - {"worker"}) < 3:
            fail("units.json.units", f"playable faction '{faction}' requires a worker and three distinct combat/support roles")
    return sorted(output, key=lambda item: item["id"])


def validate_buildings(
    root: dict[str, Any], faction_ids: set[str], playable: set[str]
) -> list[dict[str, Any]]:
    records = require_schema(root, "buildings", "buildings.json")
    output: list[dict[str, Any]] = []
    ids: set[str] = set()
    roles_by_faction: dict[str, set[str]] = {identifier: set() for identifier in playable}
    for index, raw in enumerate(records):
        path = f"buildings.json.buildings[{index}]"
        record = require_object(raw, path)
        require_exact_keys(
            record,
            {"id", "display_name", "faction", "role", "cost", "max_health", "sight_cm", "construction_ticks", "logistics_capacity", "footprint_cells"},
            {"migration"},
            path,
        )
        identifier = require_id(record["id"], f"{path}.id")
        if identifier in ids:
            fail(f"{path}.id", f"duplicate building id '{identifier}'")
        ids.add(identifier)
        faction = require_id(record["faction"], f"{path}.faction")
        if faction not in faction_ids:
            fail(f"{path}.faction", f"unknown faction '{faction}'")
        if faction not in playable:
            fail(f"{path}.faction", "buildings for a non-playable slice faction are not accepted")
        role = require_id(record["role"], f"{path}.role")
        roles_by_faction[faction].add(role)
        footprint = require_array(record["footprint_cells"], f"{path}.footprint_cells")
        if len(footprint) != 2:
            fail(f"{path}.footprint_cells", "must contain width and height")
        migration: dict[str, int] | None = None
        if "migration" in record:
            raw_migration = require_object(record["migration"], f"{path}.migration")
            require_exact_keys(
                raw_migration,
                {"move_speed_cm_s", "uproot_ticks", "root_ticks", "mobile_damage_taken_percent"},
                set(),
                f"{path}.migration",
            )
            if faction != "kharuun_assemblies" or role != "mobile_supply_node":
                fail(f"{path}.migration", "schema 1 migration is reserved for the Kharuun mobile supply node")
            migration = {
                "move_speed_cm_s": require_int(raw_migration["move_speed_cm_s"], f"{path}.migration.move_speed_cm_s", 1, 1000),
                "uproot_ticks": require_int(raw_migration["uproot_ticks"], f"{path}.migration.uproot_ticks", 1, 100_000),
                "root_ticks": require_int(raw_migration["root_ticks"], f"{path}.migration.root_ticks", 1, 100_000),
                "mobile_damage_taken_percent": require_int(raw_migration["mobile_damage_taken_percent"], f"{path}.migration.mobile_damage_taken_percent", 101, 300),
            }
        if faction == "kharuun_assemblies" and role == "mobile_supply_node" and migration is None:
            fail(f"{path}.migration", "the Kharuun mobile supply node requires authored migration rules")
        output.append(
            {
                "id": identifier,
                "display_name": require_text(record["display_name"], f"{path}.display_name"),
                "faction": faction,
                "role": role,
                "cost": require_cost(record["cost"], f"{path}.cost"),
                "max_health": require_int(record["max_health"], f"{path}.max_health", 1, 10_000_000),
                "sight_cm": require_int(record["sight_cm"], f"{path}.sight_cm", 100, 100_000),
                "construction_ticks": require_int(record["construction_ticks"], f"{path}.construction_ticks", 1, 1_000_000),
                "logistics_capacity": require_int(record["logistics_capacity"], f"{path}.logistics_capacity", 0, 100_000),
                "footprint_cells": [
                    require_int(footprint[0], f"{path}.footprint_cells[0]", 1, 64),
                    require_int(footprint[1], f"{path}.footprint_cells[1]", 1, 64),
                ],
                "migration": migration,
            }
        )
    for faction in sorted(playable):
        roles = roles_by_faction[faction]
        if "headquarters_dropoff" not in roles:
            fail("buildings.json.buildings", f"playable faction '{faction}' requires headquarters_dropoff")
        if not ({"supply_node", "mobile_supply_node"} & roles):
            fail("buildings.json.buildings", f"playable faction '{faction}' requires a logistics structure")
        if "production" not in roles:
            fail("buildings.json.buildings", f"playable faction '{faction}' requires a production structure")
        if len(roles) < 4:
            fail("buildings.json.buildings", f"playable faction '{faction}' requires four distinct structure roles")
    return sorted(output, key=lambda item: item["id"])


def validate_future_wells(root: dict[str, Any]) -> dict[str, Any]:
    require_exact_keys(root, {"schema_version", "rules"}, set(), "future_wells.json")
    require_int(root["schema_version"], "future_wells.json.schema_version", 1, 1)
    rules = require_object(root["rules"], "future_wells.json.rules")
    require_exact_keys(rules, {"capture_radius_cm", "capture_ticks", "harvest", "preserve", "reshape"}, set(), "future_wells.json.rules")
    harvest = require_object(rules["harvest"], "future_wells.json.rules.harvest")
    preserve = require_object(rules["preserve"], "future_wells.json.rules.preserve")
    reshape = require_object(rules["reshape"], "future_wells.json.rules.reshape")
    require_exact_keys(harvest, {"immediate_dawn", "permanent_state", "telegraph_ticks"}, set(), "future_wells.json.rules.harvest")
    require_exact_keys(preserve, {"dawn_per_interval", "interval_ticks", "vision_radius_cm"}, set(), "future_wells.json.rules.preserve")
    require_exact_keys(reshape, {"dawn_cost", "manifest_duration_ticks", "telegraph_ticks"}, set(), "future_wells.json.rules.reshape")
    permanent_state = require_id(harvest["permanent_state"], "future_wells.json.rules.harvest.permanent_state")
    if permanent_state != "collapsed":
        fail("future_wells.json.rules.harvest.permanent_state", "schema 1 requires 'collapsed'")
    return {
        "capture_radius_cm": require_int(rules["capture_radius_cm"], "future_wells.json.rules.capture_radius_cm", 1, 100_000),
        "capture_ticks": require_int(rules["capture_ticks"], "future_wells.json.rules.capture_ticks", 1, 1_000_000),
        "harvest": {
            "immediate_dawn": require_int(harvest["immediate_dawn"], "future_wells.json.rules.harvest.immediate_dawn", 1, 1_000_000),
            "permanent_state": permanent_state,
            "telegraph_ticks": require_int(harvest["telegraph_ticks"], "future_wells.json.rules.harvest.telegraph_ticks", 1, 1_000_000),
        },
        "preserve": {
            "dawn_per_interval": require_int(preserve["dawn_per_interval"], "future_wells.json.rules.preserve.dawn_per_interval", 1, 1_000_000),
            "interval_ticks": require_int(preserve["interval_ticks"], "future_wells.json.rules.preserve.interval_ticks", 1, 1_000_000),
            "vision_radius_cm": require_int(preserve["vision_radius_cm"], "future_wells.json.rules.preserve.vision_radius_cm", 1, 100_000),
        },
        "reshape": {
            "dawn_cost": require_int(reshape["dawn_cost"], "future_wells.json.rules.reshape.dawn_cost", 1, 1_000_000),
            "manifest_duration_ticks": require_int(reshape["manifest_duration_ticks"], "future_wells.json.rules.reshape.manifest_duration_ticks", 1, 10_000_000),
            "telegraph_ticks": require_int(reshape["telegraph_ticks"], "future_wells.json.rules.reshape.telegraph_ticks", 1, 1_000_000),
        },
    }


def build_pack(source_dir: Path) -> dict[str, Any]:
    roots = {filename: load_json(source_dir, filename) for filename in SOURCE_FILES}
    factions, playable = validate_factions(roots["factions.json"])
    faction_ids = {record["id"] for record in factions}
    return {
        "pack_format": PACK_FORMAT,
        "pack_version": PACK_VERSION,
        "schema_version": SCHEMA_VERSION,
        "factions": factions,
        "units": validate_units(roots["units.json"], faction_ids, playable),
        "buildings": validate_buildings(roots["buildings.json"], faction_ids, playable),
        "future_wells": validate_future_wells(roots["future_wells.json"]),
    }


def canonical_bytes(pack: dict[str, Any]) -> bytes:
    return (json.dumps(pack, ensure_ascii=True, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def compile_pack(source_dir: Path, output: Path, digest_output: Path | None = None) -> str:
    data = canonical_bytes(build_pack(source_dir))
    digest = hashlib.sha256(data).hexdigest()
    atomic_write(output, data)
    digest_path = digest_output or output.with_suffix(output.suffix + ".sha256")
    atomic_write(digest_path, f"{digest}\n".encode("ascii"))
    return digest


def parse_arguments() -> argparse.Namespace:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=root / "Content/Data/Source")
    parser.add_argument("--output", type=Path, default=root / "Content/Data/Generated/EchoesContentPack.json")
    parser.add_argument("--digest-output", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        digest = compile_pack(arguments.source, arguments.output, arguments.digest_output)
    except ContentValidationError as error:
        print(f"Content validation failed: {error}", file=os.sys.stderr)
        return 2
    print(f"Authoritative content validated: {arguments.output}")
    print(f"Content SHA-256: {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
