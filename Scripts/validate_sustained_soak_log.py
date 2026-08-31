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
STABILIZATION_RESET_MARKER = "[ECHOES_STRESS_SUSTAINED_STABILIZATION_RESET]"
STABILIZED_MARKER = "[ECHOES_STRESS_SUSTAINED_STABILIZED]"
HEARTBEAT_MARKER = "[ECHOES_STRESS_SUSTAINED_HEARTBEAT]"
MEMORY_MARKER = "[ECHOES_STRESS_SUSTAINED_MEMORY]"
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
    "memoryPoolSchema",
    "memoryTelemetryIntervalTicks",
    "entityFreeCapacity",
    "effectsQuality",
    "destructionCapacity",
    "initialCommandLog",
    "commandCapacity",
}

STABILIZATION_RESET_KEYS = {
    "tick",
    "rawDeltaUs",
    "stableFramesBeforeReset",
    "stableWallUsBeforeReset",
    "maximumDeltaUs",
}

STABILIZED_KEYS = {
    "tick",
    "stableFrames",
    "stableWallUs",
    "minimumStableFrames",
    "minimumStableWallUs",
    "maximumDeltaUs",
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

MEMORY_KEYS = {
    "schema",
    "fixture",
    "tick",
    "wall_ms",
    "sequence",
    "intervalTicks",
    "processUsedPhysicalBytes",
    "processPeakUsedPhysicalBytes",
    "globalObjectSlots",
    "globalObjectClaimed",
    "gcCycles",
    "gcLastPreUsedPhysicalBytes",
    "gcLastPostUsedPhysicalBytes",
    "gcLastUsedPhysicalDeltaBytes",
    "gcLastObjectSlotDelta",
    "gcLastClaimedObjectSlotDelta",
    "entityActive",
    "entityFree",
    "entityFreeCapacity",
    "entityCreated",
    "entityReused",
    "entityReleased",
    "entityOverflow",
    "effectsQuality",
    "destructionActive",
    "destructionFree",
    "destructionCapacity",
    "destructionCreated",
    "destructionReused",
    "destructionActivated",
    "destructionReleased",
    "destructionOverflow",
    "destructionCoalesced",
    "entityMIDCreated",
    "destructionMIDCreated",
    "commandLog",
    "commandLimit",
    "commandLogCapacity",
    "commandElementBytes",
    "commandLogAllocatedBytes",
    "cumulativeReplacements",
    "naturalGc",
    "forcedGc",
    "authoritative",
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


def _as_signed_int(fields: dict[str, str], key: str) -> int:
    try:
        return int(fields[key], 10)
    except ValueError as exc:
        raise ValidationError(f"field {key} is not a base-10 integer") from exc


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
    stabilized_records = _records(lines, STABILIZED_MARKER, STABILIZED_KEYS)
    if len(stabilized_records) != 1:
        raise ValidationError(
            f"expected exactly one stabilization marker, observed {len(stabilized_records)}"
        )
    stabilized_index, stabilized = stabilized_records[0]
    _require_exact(
        stabilized,
        {
            "tick": 0,
            "minimumStableFrames": 20,
            "minimumStableWallUs": 1_000_000,
            "maximumDeltaUs": 250_000,
        },
    )
    stable_frames = _as_int(stabilized, "stableFrames")
    stable_wall_us = _as_int(stabilized, "stableWallUs")
    if stable_frames < 20 or stable_wall_us < 1_000_000:
        raise ValidationError("stabilization marker is below its declared thresholds")
    if stable_wall_us > stable_frames * 250_000:
        raise ValidationError("stabilization counters exceed the per-frame time bound")
    if stable_frames > 20 and stable_wall_us > 1_250_000:
        raise ValidationError("stabilization did not occur on the first valid threshold crossing")

    reset_records = _records(
        lines, STABILIZATION_RESET_MARKER, STABILIZATION_RESET_KEYS
    )
    for reset_index, reset in reset_records:
        _require_exact(reset, {"tick": 0, "maximumDeltaUs": 250_000})
        if _as_int(reset, "rawDeltaUs") <= 250_000:
            raise ValidationError("startup reset delta did not exceed the declared maximum")
        reset_frames = _as_int(reset, "stableFramesBeforeReset")
        reset_wall_us = _as_int(reset, "stableWallUsBeforeReset")
        if reset_wall_us > reset_frames * 250_000:
            raise ValidationError("startup reset counters exceed the per-frame time bound")
        if reset_frames >= 20 and reset_wall_us >= 1_000_000:
            raise ValidationError("startup reset was recorded after stabilization was due")
        if reset_index >= stabilized_index:
            raise ValidationError("a startup reset occurred after stabilization")

    ready_records = _records(lines, READY_MARKER, READY_KEYS)
    if len(ready_records) != 1:
        raise ValidationError(f"expected exactly one readiness marker, observed {len(ready_records)}")
    ready_index, ready = ready_records[0]
    if stabilized_index >= ready_index:
        raise ValidationError("sustained readiness preceded startup stabilization")
    _require_exact(ready, EXACT_COMMON)
    _require_exact(
        ready,
        {
            "tick": 0,
            "tickRate": 20,
            "protectedCoreMask": 15,
            "memoryPoolSchema": 2,
            "memoryTelemetryIntervalTicks": 200,
            "entityFreeCapacity": 512,
            "initialCommandLog": 396,
            "commandCapacity": 262_144,
        },
    )
    effects_quality = _as_int(ready, "effectsQuality")
    if effects_quality > 3:
        raise ValidationError("readiness effects quality is outside 0 through 3")
    expected_destruction_capacity = (
        64 if effects_quality == 0 else 160 if effects_quality == 1 else 396
    )
    if _as_int(ready, "destructionCapacity") != expected_destruction_capacity:
        raise ValidationError("readiness destruction capacity does not match effects quality")
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

    memory_records = _records(lines, MEMORY_MARKER, MEMORY_KEYS)
    expected_memory_ticks = list(range(0, previous_tick + 1, 200))
    if len(memory_records) != len(expected_memory_ticks):
        raise ValidationError(
            "memory telemetry count drifted: "
            f"expected {len(expected_memory_ticks)}, observed {len(memory_records)}"
        )
    previous_memory: dict[str, int] | None = None
    baseline_entity_mid_count: int | None = None
    warm_plateau: tuple[int, int] | None = None
    for position, (memory_index, memory) in enumerate(memory_records):
        expected_tick = expected_memory_ticks[position]
        _require_exact(
            memory,
            {
                "schema": 2,
                "fixture": "Stress400Sustained",
                "tick": expected_tick,
                "sequence": position,
                "intervalTicks": 200,
                "entityActive": 401,
                "entityFree": 0,
                "entityFreeCapacity": 512,
                "entityCreated": 401,
                "entityOverflow": 0,
                "effectsQuality": effects_quality,
                "destructionCapacity": expected_destruction_capacity,
                "destructionOverflow": 0,
                "destructionCoalesced": 0,
                "commandLimit": 262_144,
                "naturalGc": "true",
                "forcedGc": "false",
                "authoritative": "false",
            },
        )
        if position == 0:
            if memory_index <= ready_index or memory_index >= heartbeat_records[0][0]:
                raise ValidationError(
                    "tick-zero memory telemetry must follow readiness and precede heartbeats"
                )
        else:
            heartbeat = heartbeat_by_tick.get(expected_tick)
            if heartbeat is None:
                raise ValidationError(
                    f"memory telemetry tick {expected_tick} has no matching heartbeat"
                )
            if memory_index <= heartbeat_line_by_tick[expected_tick]:
                raise ValidationError(
                    f"memory telemetry tick {expected_tick} preceded its heartbeat"
                )
            if _as_int(memory, "wall_ms") != _as_int(heartbeat, "wall_ms"):
                raise ValidationError(
                    f"memory telemetry tick {expected_tick} wall time differs from heartbeat"
                )
            if _as_int(memory, "commandLog") != _as_int(heartbeat, "commandLog"):
                raise ValidationError(
                    f"memory telemetry tick {expected_tick} command log differs from heartbeat"
                )
            if _as_int(memory, "cumulativeReplacements") != _as_int(
                heartbeat, "cumulativeReplacements"
            ):
                raise ValidationError(
                    f"memory telemetry tick {expected_tick} replacement count differs from heartbeat"
                )

        wall_ms = _as_int(memory, "wall_ms")
        if position == 0 and wall_ms != 0:
            raise ValidationError("tick-zero memory telemetry wall time must be zero")
        used_physical = _as_int(memory, "processUsedPhysicalBytes")
        peak_used_physical = _as_int(memory, "processPeakUsedPhysicalBytes")
        if used_physical == 0 or peak_used_physical == 0:
            raise ValidationError("memory telemetry process counters must be nonzero")
        if used_physical > peak_used_physical:
            raise ValidationError("process used memory exceeds process peak memory")
        global_slots = _as_int(memory, "globalObjectSlots")
        global_claimed = _as_int(memory, "globalObjectClaimed")
        if global_slots == 0 or global_claimed == 0 or global_claimed > global_slots:
            raise ValidationError("global UObject slot accounting is invalid")

        gc_cycles = _as_int(memory, "gcCycles")
        gc_pre = _as_int(memory, "gcLastPreUsedPhysicalBytes")
        gc_post = _as_int(memory, "gcLastPostUsedPhysicalBytes")
        gc_used_delta = _as_signed_int(memory, "gcLastUsedPhysicalDeltaBytes")
        gc_slot_delta = _as_signed_int(memory, "gcLastObjectSlotDelta")
        gc_claimed_delta = _as_signed_int(memory, "gcLastClaimedObjectSlotDelta")
        if gc_cycles == 0 and any(
            value != 0
            for value in (
                gc_pre,
                gc_post,
                gc_used_delta,
                gc_slot_delta,
                gc_claimed_delta,
            )
        ):
            raise ValidationError("GC detail counters changed before a natural GC cycle")
        if gc_cycles > 0:
            if gc_pre == 0 or gc_post == 0:
                raise ValidationError(
                    "natural GC memory telemetry is missing its pre/post counters"
                )
            if gc_used_delta != gc_post - gc_pre:
                raise ValidationError(
                    "natural GC used-memory delta does not reconcile with pre/post counters"
                )

        replacements = _as_int(memory, "cumulativeReplacements")
        entity_reused = _as_int(memory, "entityReused")
        entity_released = _as_int(memory, "entityReleased")
        if entity_reused != replacements or entity_released != replacements:
            raise ValidationError(
                "entity retire/reuse counters do not exactly reconcile with replacements"
            )
        entity_mid_count = _as_int(memory, "entityMIDCreated")
        if entity_mid_count == 0:
            raise ValidationError("entity MID ownership evidence is empty")
        if baseline_entity_mid_count is None:
            baseline_entity_mid_count = entity_mid_count
        elif entity_mid_count != baseline_entity_mid_count:
            raise ValidationError("entity MID creation did not plateau at readiness")

        destruction_active = _as_int(memory, "destructionActive")
        destruction_free = _as_int(memory, "destructionFree")
        destruction_created = _as_int(memory, "destructionCreated")
        destruction_reused = _as_int(memory, "destructionReused")
        destruction_activated = _as_int(memory, "destructionActivated")
        destruction_released = _as_int(memory, "destructionReleased")
        destruction_mid_count = _as_int(memory, "destructionMIDCreated")
        if destruction_active + destruction_free != destruction_created:
            raise ValidationError("destruction actor pool accounting is not closed")
        if destruction_created > expected_destruction_capacity:
            raise ValidationError("destruction actor pool exceeded its quality-tier capacity")
        if destruction_activated != replacements:
            raise ValidationError(
                "destruction activation does not reconcile with authoritative replacements"
            )
        if destruction_activated != destruction_created + destruction_reused:
            raise ValidationError("destruction acquire accounting is not closed")
        if destruction_released != destruction_activated - destruction_active:
            raise ValidationError("destruction release accounting is not closed")
        if destruction_free != destruction_released - destruction_reused:
            raise ValidationError("destruction free-pool accounting is not closed")
        if destruction_mid_count != destruction_created * 4:
            raise ValidationError("destruction MID ownership is not four per actor")

        command_log = _as_int(memory, "commandLog")
        if position == 0:
            if command_log != _as_int(ready, "initialCommandLog") or replacements != 0:
                raise ValidationError("tick-zero command or replacement baseline drifted")
        command_capacity = _as_int(memory, "commandLogCapacity")
        command_element_bytes = _as_int(memory, "commandElementBytes")
        command_allocated_bytes = _as_int(memory, "commandLogAllocatedBytes")
        if (
            command_element_bytes == 0
            or command_element_bytes > 4_096
            or command_capacity < command_log
            or command_capacity > 262_144
            or command_allocated_bytes != command_capacity * command_element_bytes
        ):
            raise ValidationError("command-log allocation accounting is invalid")

        current_memory = {
            "wall_ms": wall_ms,
            "peak": peak_used_physical,
            "gc_cycles": gc_cycles,
            "gc_pre": gc_pre,
            "gc_post": gc_post,
            "gc_used_delta": gc_used_delta,
            "gc_slot_delta": gc_slot_delta,
            "gc_claimed_delta": gc_claimed_delta,
            "entity_reused": entity_reused,
            "entity_released": entity_released,
            "destruction_created": destruction_created,
            "destruction_reused": destruction_reused,
            "destruction_activated": destruction_activated,
            "destruction_released": destruction_released,
            "command_capacity": command_capacity,
            "command_element_bytes": command_element_bytes,
        }
        if previous_memory is not None:
            for key in (
                "wall_ms",
                "peak",
                "gc_cycles",
                "entity_reused",
                "entity_released",
                "destruction_created",
                "destruction_reused",
                "destruction_activated",
                "destruction_released",
                "command_capacity",
            ):
                if current_memory[key] < previous_memory[key]:
                    raise ValidationError(f"memory telemetry counter {key} regressed")
            if current_memory["command_element_bytes"] != previous_memory[
                "command_element_bytes"
            ]:
                raise ValidationError("command element size changed during the run")
            if current_memory["gc_cycles"] == previous_memory["gc_cycles"]:
                for key in (
                    "gc_pre",
                    "gc_post",
                    "gc_used_delta",
                    "gc_slot_delta",
                    "gc_claimed_delta",
                ):
                    if current_memory[key] != previous_memory[key]:
                        raise ValidationError(
                            "GC detail changed without a natural collection count change"
                        )
        previous_memory = current_memory

        if expected_tick >= 2_400:
            plateau = (
                destruction_created,
                destruction_mid_count,
            )
            if warm_plateau is None:
                warm_plateau = plateau
            elif plateau != warm_plateau:
                raise ValidationError(
                    "post-warmup destruction pool or MID count did not plateau"
                )

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
            or qualified_index <= memory_records[-1][0]
        ):
            raise ValidationError(
                "qualification was not emitted after its validated heartbeat and memory telemetry"
            )
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
        "startup_reset_count": len(reset_records),
        "startup_stable_frames": stable_frames,
        "startup_stable_wall_us": stable_wall_us,
        "heartbeat_count": len(heartbeat_records),
        "memory_telemetry_count": len(memory_records),
        "final_memory_telemetry_tick": expected_memory_ticks[-1],
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
