#!/usr/bin/env python3
"""Fail-closed validation for the packaged sustained-stress runtime log."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
from typing import Iterable


READY_MARKER = "[ECHOES_STRESS_SUSTAINED_READY]"
HEARTBEAT_MARKER = "[ECHOES_STRESS_SUSTAINED_HEARTBEAT]"
QUALIFIED_MARKER = "[ECHOES_STRESS_SUSTAINED_QUALIFIED]"

FORBIDDEN_MARKERS = (
    "[ECHOES_STRESS_SUSTAINED_FAILED]",
    "[ECHOES_MATCH_FINISHED]",
    "[ECHOES_MATCH_PAUSE] paused=true",
    "[ECHOES_SIM_TIME_CLAMP]",
    "[ECHOES_SIM_VIEW_SYNC_FAILED]",
    "[ECHOES_BOOT_INCOMPLETE]",
    "[ECHOES_BOOT_NO_SUBSYSTEM]",
    "[ECHOES_CONTENT_FAILED]",
    "[ECHOES_SIM_CONTENT_REJECTED]",
    "[ECHOES_FOG_INIT_FAILED]",
    "[ECHOES_TERRAIN_VIEW_INIT_FAILED]",
    "[ECHOES_STRESS_READY]",
    "[ECHOES_STRESS_ORDERS_READY]",
    "[ECHOES_STRESS_COMBAT_ACTIVE]",
)

FORBIDDEN_CASEFOLDED_MARKERS = (
    "fatal error:",
    "lowlevelfatalerror",
    "assertion failed:",
    "gpu crashed",
    "out of memory",
    "ran out of memory",
    "out of video memory",
    "segmentation fault",
    "signal 11",
    "ensure condition failed",
    "unhandled exception",
    "sigabrt",
    "sigbus",
    "signal 6",
    "signal 10",
)

READY_KEYS = {
    "fixture",
    "tick",
    "checksum",
    "outcome",
    "activePlayers",
    "activeFactions",
    "meridian",
    "kharuun",
    "hollowChoir",
    "team0",
    "team1",
    "team2",
    "team3",
    "commandCores",
    "combatUnits",
    "ownedEntities",
    "neutralWells",
    "entities",
    "views",
    "tickRate",
    "protectedCoreMask",
}

HEARTBEAT_KEYS = {
    "fixture",
    "tick",
    "wall_ms",
    "checksum",
    "outcome",
    "activePlayers",
    "activeFactions",
    "meridian",
    "kharuun",
    "hollowChoir",
    "team0",
    "team1",
    "team2",
    "team3",
    "commandCores",
    "soldiers",
    "heavies",
    "scouts",
    "combatUnits",
    "ownedEntities",
    "neutralWells",
    "entities",
    "views",
    "damagedCombatants",
    "activeAttackMove",
    "activityAgeTicks",
    "activityWindowTicks",
    "intervalDamage",
    "intervalCombatLosses",
    "cumulativeCombatLosses",
    "intervalReplacements",
    "cumulativeReplacements",
    "intervalOrderRenewals",
    "cumulativeOrderRenewals",
    "commandLog",
    "commandCapacity",
    "replacementBudget",
    "renewalBudget",
    "projectedCommandCeiling",
    "commandSafetyReserve",
    "qualificationTicks",
}

QUALIFIED_KEYS = {
    "fixture",
    "tick",
    "checksum",
    "outcome",
    "combatUnits",
    "ownedEntities",
    "entities",
    "views",
    "cumulativeCombatLosses",
    "cumulativeReplacements",
    "cumulativeOrderRenewals",
    "commandLog",
    "commandCapacity",
}

EXACT_COMMON = {
    "fixture": "Stress400Sustained",
    "outcome": "ongoing",
    "activePlayers": 4,
    "activeFactions": 3,
    "meridian": 200,
    "kharuun": 100,
    "hollowChoir": 100,
    "team0": 100,
    "team1": 100,
    "team2": 100,
    "team3": 100,
    "commandCores": 4,
    "combatUnits": 396,
    "ownedEntities": 400,
    "neutralWells": 1,
    "entities": 401,
    "views": 401,
}

EXACT_BUDGET = {
    "activityWindowTicks": 100,
    "commandCapacity": 262_144,
    "replacementBudget": 200_000,
    "renewalBudget": 14_400,
    "projectedCommandCeiling": 214_796,
    "commandSafetyReserve": 47_348,
    "qualificationTicks": 72_000,
}


class ValidationError(RuntimeError):
    pass


UNREAL_LOG_PREFIX = re.compile(
    r"(?:\[[^\]\r\n]+\]\[\s*\d+\])?LogEchoes: (?:Display|Warning|Error): "
)


def _parse_fields(line: str, marker: str, required_keys: set[str]) -> dict[str, str]:
    if line.count(marker) != 1:
        raise ValidationError(f"invalid occurrence of {marker}")
    prefix, payload = line.split(marker, 1)
    if prefix and UNREAL_LOG_PREFIX.fullmatch(prefix) is None:
        raise ValidationError(f"unrecognized runtime-log prefix before {marker}")
    payload = payload.strip()
    fields: dict[str, str] = {}
    for token in payload.split():
        if "=" not in token:
            raise ValidationError(f"malformed {marker} token: {token!r}")
        key, value = token.split("=", 1)
        if not key or not value or key in fields:
            raise ValidationError(f"invalid or duplicate {marker} field: {token!r}")
        fields[key] = value
    if set(fields) != required_keys:
        missing = sorted(required_keys - set(fields))
        unexpected = sorted(set(fields) - required_keys)
        raise ValidationError(
            f"{marker} field schema mismatch; missing={missing}, unexpected={unexpected}"
        )
    return fields


def _as_int(fields: dict[str, str], key: str) -> int:
    try:
        value = int(fields[key], 10)
    except ValueError as exc:
        raise ValidationError(f"field {key} is not a base-10 integer") from exc
    if value < 0:
        raise ValidationError(f"field {key} must be nonnegative")
    return value


def _require_exact(fields: dict[str, str], expected: dict[str, object]) -> None:
    for key, value in expected.items():
        observed: object = fields[key] if isinstance(value, str) else _as_int(fields, key)
        if observed != value:
            raise ValidationError(f"field {key} drifted: expected {value}, observed {observed}")


def _records(
    lines: Iterable[str], marker: str, required_keys: set[str]
) -> list[tuple[int, dict[str, str]]]:
    return [
        (index, _parse_fields(line, marker, required_keys))
        for index, line in enumerate(lines)
        if marker in line
    ]


def validate_log(text: str, duration_seconds: int) -> dict[str, object]:
    if duration_seconds <= 0:
        raise ValidationError("duration_seconds must be positive")
    for forbidden in FORBIDDEN_MARKERS:
        if forbidden in text:
            raise ValidationError(f"forbidden runtime marker present: {forbidden}")
    casefolded_text = text.casefold()
    generic_failure = re.search(r"\[ECHOES_[A-Z0-9_]*FAILED\]", text)
    if generic_failure is not None:
        raise ValidationError(
            f"generic project failure marker present: {generic_failure.group(0)}"
        )
    for forbidden in FORBIDDEN_CASEFOLDED_MARKERS:
        if forbidden in casefolded_text:
            raise ValidationError(f"forbidden fatal marker present: {forbidden}")

    lines = text.splitlines()
    ready_records = _records(lines, READY_MARKER, READY_KEYS)
    if len(ready_records) != 1:
        raise ValidationError(f"expected exactly one readiness marker, observed {len(ready_records)}")
    ready_index, ready = ready_records[0]
    _require_exact(ready, EXACT_COMMON)
    _require_exact(
        ready,
        {"tick": 0, "tickRate": 20, "protectedCoreMask": 15},
    )
    if _as_int(ready, "checksum") == 0:
        raise ValidationError("readiness checksum must be nonzero")

    heartbeat_records = _records(lines, HEARTBEAT_MARKER, HEARTBEAT_KEYS)
    if not heartbeat_records:
        raise ValidationError("no sustained heartbeat was present")
    if heartbeat_records[0][0] <= ready_index:
        raise ValidationError("a heartbeat preceded sustained readiness")

    previous_tick = 0
    previous_wall_ms = 0
    previous_losses = 0
    previous_replacements = 0
    previous_renewals = 0
    total_interval_damage = 0
    heartbeat_by_tick: dict[int, dict[str, str]] = {}
    heartbeat_line_by_tick: dict[int, int] = {}
    for heartbeat_index, heartbeat in heartbeat_records:
        _require_exact(heartbeat, EXACT_COMMON)
        _require_exact(heartbeat, EXACT_BUDGET)
        _require_exact(heartbeat, {"soldiers": 132, "heavies": 132, "scouts": 132})

        tick = _as_int(heartbeat, "tick")
        wall_ms = _as_int(heartbeat, "wall_ms")
        if tick != previous_tick + 20:
            raise ValidationError(
                f"heartbeat cadence drifted: previous tick {previous_tick}, observed {tick}"
            )
        if wall_ms <= previous_wall_ms:
            raise ValidationError("heartbeat wall_ms is duplicate or nonmonotonic")
        if wall_ms - previous_wall_ms > 2_000:
            raise ValidationError("heartbeat wall-time gap exceeded 2 seconds")
        if tick in heartbeat_by_tick:
            raise ValidationError(f"duplicate heartbeat tick: {tick}")
        heartbeat_by_tick[tick] = heartbeat
        heartbeat_line_by_tick[tick] = heartbeat_index

        activity_age = _as_int(heartbeat, "activityAgeTicks")
        if activity_age > _as_int(heartbeat, "activityWindowTicks"):
            raise ValidationError("heartbeat exceeded the rolling combat-activity window")
        active_attack_move = _as_int(heartbeat, "activeAttackMove")
        if active_attack_move == 0 or active_attack_move > 396:
            raise ValidationError("heartbeat active AttackMove count is outside 1 through 396")
        if _as_int(heartbeat, "damagedCombatants") > 396:
            raise ValidationError("heartbeat damaged-combatant count exceeds 396")

        interval_damage = _as_int(heartbeat, "intervalDamage")
        interval_losses = _as_int(heartbeat, "intervalCombatLosses")
        cumulative_losses = _as_int(heartbeat, "cumulativeCombatLosses")
        interval_replacements = _as_int(heartbeat, "intervalReplacements")
        cumulative_replacements = _as_int(heartbeat, "cumulativeReplacements")
        interval_renewals = _as_int(heartbeat, "intervalOrderRenewals")
        cumulative_renewals = _as_int(heartbeat, "cumulativeOrderRenewals")
        if interval_renewals > 4 or cumulative_renewals > 14_400:
            raise ValidationError("order-renewal accounting exceeds its one-hour budget")
        if cumulative_replacements > 200_000:
            raise ValidationError("replacement accounting exceeds its one-hour budget")
        if interval_losses != interval_replacements or cumulative_losses != cumulative_replacements:
            raise ValidationError("combat losses and replacements diverged")
        if cumulative_losses != previous_losses + interval_losses:
            raise ValidationError("cumulative combat-loss accounting drifted")
        if cumulative_replacements != previous_replacements + interval_replacements:
            raise ValidationError("cumulative replacement accounting drifted")
        if cumulative_renewals != previous_renewals + interval_renewals:
            raise ValidationError("cumulative renewal accounting drifted")

        command_log = _as_int(heartbeat, "commandLog")
        expected_commands = 396 + cumulative_replacements + cumulative_renewals
        if command_log != expected_commands:
            raise ValidationError(
                f"command-log accounting drifted: expected {expected_commands}, observed {command_log}"
            )
        if command_log >= _as_int(heartbeat, "commandCapacity"):
            raise ValidationError("command log reached or exceeded capacity")
        if command_log > _as_int(heartbeat, "projectedCommandCeiling"):
            raise ValidationError("command log exceeded the one-hour projected ceiling")
        if _as_int(heartbeat, "checksum") == 0:
            raise ValidationError("heartbeat checksum must be nonzero")
        if previous_tick != 0 and _as_int(heartbeat, "checksum") == _as_int(
            heartbeat_by_tick[previous_tick], "checksum"
        ):
            raise ValidationError("successive heartbeat checksums did not change")

        total_interval_damage += interval_damage
        previous_tick = tick
        previous_wall_ms = wall_ms
        previous_losses = cumulative_losses
        previous_replacements = cumulative_replacements
        previous_renewals = cumulative_renewals

    minimum_tick = duration_seconds * 20
    minimum_wall_ms = max(1, duration_seconds * 1_000 - 1_000)
    if previous_tick < minimum_tick:
        raise ValidationError(
            f"final simulated tick {previous_tick} is below required tick {minimum_tick}"
        )
    if previous_wall_ms < minimum_wall_ms:
        raise ValidationError(
            f"final heartbeat wall_ms {previous_wall_ms} is below active-duration boundary {minimum_wall_ms}"
        )
    if total_interval_damage == 0:
        raise ValidationError("no deterministic combat damage was observed")
    if previous_replacements == 0:
        raise ValidationError("no deterministic loss-and-replacement cycle was observed")

    qualified_records = _records(lines, QUALIFIED_MARKER, QUALIFIED_KEYS)
    if len(qualified_records) > 1:
        raise ValidationError("multiple qualification markers were present")
    if duration_seconds >= 3_600 and len(qualified_records) != 1:
        raise ValidationError("the one-hour run is missing its qualification marker")
    if qualified_records:
        qualified_index, qualified = qualified_records[0]
        _require_exact(
            qualified,
            {
                "fixture": "Stress400Sustained",
                "tick": 72_000,
                "outcome": "ongoing",
                "combatUnits": 396,
                "ownedEntities": 400,
                "entities": 401,
                "views": 401,
                "commandCapacity": 262_144,
            },
        )
        if (
            72_000 not in heartbeat_by_tick
            or qualified_index <= heartbeat_line_by_tick[72_000]
        ):
            raise ValidationError("qualification was not emitted after its validated heartbeat")
        boundary = heartbeat_by_tick[72_000]
        for key in (
            "checksum",
            "cumulativeCombatLosses",
            "cumulativeReplacements",
            "cumulativeOrderRenewals",
            "commandLog",
        ):
            if _as_int(qualified, key) != _as_int(boundary, key):
                raise ValidationError(f"qualification field {key} differs from tick-72000 heartbeat")

    return {
        "accepted": True,
        "duration_seconds": duration_seconds,
        "heartbeat_count": len(heartbeat_records),
        "first_tick": 20,
        "final_tick": previous_tick,
        "final_wall_ms": previous_wall_ms,
        "final_checksum": _as_int(heartbeat_records[-1][1], "checksum"),
        "cumulative_combat_losses": previous_losses,
        "cumulative_replacements": previous_replacements,
        "cumulative_order_renewals": previous_renewals,
        "final_command_log": _as_int(heartbeat_records[-1][1], "commandLog"),
        "qualified_one_hour": bool(qualified_records),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", required=True, type=pathlib.Path)
    parser.add_argument("--duration-seconds", required=True, type=int)
    parser.add_argument("--json-output", type=pathlib.Path)
    args = parser.parse_args()
    try:
        text = args.log.read_text(encoding="utf-8", errors="replace")
        result = validate_log(text, args.duration_seconds)
    except (OSError, ValidationError) as exc:
        print(f"SUSTAINED_SOAK_VALIDATION_FAILED: {exc}", file=sys.stderr)
        return 1
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json_output is not None:
        args.json_output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
