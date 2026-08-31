#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import sys
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT_ROOT / "Scripts"))

from validate_sustained_soak_log import ValidationError, validate_log  # noqa: E402


READY = (
    "[ECHOES_STRESS_SUSTAINED_READY] fixture=Stress400Sustained tick=0 "
    "checksum=111 outcome=ongoing activePlayers=4 activeFactions=3 "
    "meridian=200 kharuun=100 hollowChoir=100 team0=100 team1=100 "
    "team2=100 team3=100 commandCores=4 combatUnits=396 "
    "ownedEntities=400 neutralWells=1 entities=401 views=401 tickRate=20 "
    "protectedCoreMask=15 memoryPoolSchema=2 memoryTelemetryIntervalTicks=200 "
    "entityFreeCapacity=512 effectsQuality=3 destructionCapacity=396 "
    "initialCommandLog=396 commandCapacity=262144"
)

STABILIZED = (
    "[ECHOES_STRESS_SUSTAINED_STABILIZED] tick=0 stableFrames=20 "
    "stableWallUs=1000000 minimumStableFrames=20 minimumStableWallUs=1000000 "
    "maximumDeltaUs=250000"
)

STABILIZATION_RESET = (
    "[ECHOES_STRESS_SUSTAINED_STABILIZATION_RESET] tick=0 rawDeltaUs=300000 "
    "stableFramesBeforeReset=7 stableWallUsBeforeReset=350000 maximumDeltaUs=250000"
)


def memory_line(
    tick: int,
    wall_ms: int,
    cumulative_replacements: int,
    command_log: int,
) -> str:
    destruction_created = min(cumulative_replacements, 2)
    destruction_active = 0
    destruction_free = destruction_created
    destruction_reused = max(0, cumulative_replacements - destruction_created)
    destruction_released = cumulative_replacements
    return (
        "[ECHOES_STRESS_SUSTAINED_MEMORY] schema=2 fixture=Stress400Sustained "
        f"tick={tick} wall_ms={wall_ms} sequence={tick // 200} intervalTicks=200 "
        "processUsedPhysicalBytes=536870912 processPeakUsedPhysicalBytes=536870912 "
        "globalObjectSlots=100000 globalObjectClaimed=90000 gcCycles=0 "
        "gcLastPreUsedPhysicalBytes=0 gcLastPostUsedPhysicalBytes=0 "
        "gcLastUsedPhysicalDeltaBytes=0 gcLastObjectSlotDelta=0 "
        "gcLastClaimedObjectSlotDelta=0 entityActive=401 entityFree=0 "
        "entityFreeCapacity=512 entityCreated=401 "
        f"entityReused={cumulative_replacements} "
        f"entityReleased={cumulative_replacements} entityOverflow=0 "
        f"effectsQuality=3 destructionActive={destruction_active} "
        f"destructionFree={destruction_free} destructionCapacity=396 "
        f"destructionCreated={destruction_created} "
        f"destructionReused={destruction_reused} "
        f"destructionActivated={cumulative_replacements} "
        f"destructionReleased={destruction_released} "
        "destructionOverflow=0 destructionCoalesced=0 entityMIDCreated=6035 "
        f"destructionMIDCreated={destruction_created * 4} "
        f"commandLog={command_log} commandLimit=262144 commandLogCapacity=512 "
        "commandElementBytes=128 commandLogAllocatedBytes=65536 "
        f"cumulativeReplacements={cumulative_replacements} "
        "naturalGc=true forcedGc=false authoritative=false"
    )


def valid_log(
    duration_seconds: int = 10,
    qualified: bool = False,
    replacement_seconds: tuple[int, ...] = (5,),
) -> str:
    lines = [
        STABILIZATION_RESET,
        STABILIZED,
        READY,
        memory_line(0, 0, 0, 396),
    ]
    cumulative_losses = 0
    cumulative_replacements = 0
    cumulative_renewals = 0
    boundary_values: tuple[int, int, int, int, int] | None = None
    for second in range(1, duration_seconds + 1):
        tick = second * 20
        interval_losses = 1 if second in replacement_seconds else 0
        interval_replacements = interval_losses
        interval_renewals = 1 if second == 6 else 0
        cumulative_losses += interval_losses
        cumulative_replacements += interval_replacements
        cumulative_renewals += interval_renewals
        checksum = 10_000 + tick
        command_log = 396 + cumulative_replacements + cumulative_renewals
        lines.append(
            "[ECHOES_STRESS_SUSTAINED_HEARTBEAT] "
            f"fixture=Stress400Sustained tick={tick} wall_ms={second * 1000} "
            f"checksum={checksum} outcome=ongoing activePlayers=4 activeFactions=3 "
            "meridian=200 kharuun=100 hollowChoir=100 team0=100 team1=100 "
            "team2=100 team3=100 commandCores=4 soldiers=132 heavies=132 "
            "scouts=132 combatUnits=396 ownedEntities=400 neutralWells=1 "
            "entities=401 views=401 damagedCombatants=12 activeAttackMove=300 "
            "activityAgeTicks=0 activityWindowTicks=100 intervalDamage=25 "
            f"intervalCombatLosses={interval_losses} "
            f"cumulativeCombatLosses={cumulative_losses} "
            f"intervalReplacements={interval_replacements} "
            f"cumulativeReplacements={cumulative_replacements} "
            f"intervalOrderRenewals={interval_renewals} "
            f"cumulativeOrderRenewals={cumulative_renewals} "
            f"commandLog={command_log} commandCapacity=262144 "
            "replacementBudget=200000 renewalBudget=14400 "
            "projectedCommandCeiling=214796 commandSafetyReserve=47348 "
            "qualificationTicks=72000"
        )
        if tick % 200 == 0:
            lines.append(
                memory_line(
                    tick,
                    second * 1000,
                    cumulative_replacements,
                    command_log,
                )
            )
        if tick == 72_000:
            boundary_values = (
                checksum,
                cumulative_losses,
                cumulative_replacements,
                cumulative_renewals,
                command_log,
            )
    if qualified:
        if boundary_values is None:
            raise ValueError("qualified logs require at least 3600 seconds")
        checksum, losses, replacements, renewals, commands = boundary_values
        lines.append(
            "[ECHOES_STRESS_SUSTAINED_QUALIFIED] "
            f"fixture=Stress400Sustained tick=72000 checksum={checksum} "
            "outcome=ongoing combatUnits=396 ownedEntities=400 entities=401 "
            f"views=401 cumulativeCombatLosses={losses} "
            f"cumulativeReplacements={replacements} "
            f"cumulativeOrderRenewals={renewals} commandLog={commands} "
            "commandCapacity=262144"
        )
    return "\n".join(lines) + "\n"


class SustainedSoakValidatorTests(unittest.TestCase):
    def assert_rejected(self, text: str, duration: int = 10) -> None:
        with self.assertRaises(ValidationError):
            validate_log(text, duration)

    def test_valid_preflight_is_accepted(self) -> None:
        result = validate_log(valid_log(), 10)
        self.assertTrue(result["accepted"])
        self.assertEqual(result["final_tick"], 200)
        self.assertEqual(result["cumulative_replacements"], 1)
        self.assertEqual(result["memory_telemetry_count"], 2)
        self.assertFalse(result["qualified_one_hour"])

    def test_valid_one_hour_boundary_is_accepted(self) -> None:
        result = validate_log(valid_log(3600, qualified=True), 3600)
        self.assertEqual(result["final_tick"], 72_000)
        self.assertEqual(result["heartbeat_count"], 3600)
        self.assertEqual(result["memory_telemetry_count"], 361)
        self.assertTrue(result["qualified_one_hour"])

    def test_memory_telemetry_is_exact_ordered_and_reconciled(self) -> None:
        valid = valid_log()
        memory_lines = [
            line
            for line in valid.splitlines()
            if "[ECHOES_STRESS_SUSTAINED_MEMORY]" in line
        ]
        self.assertEqual(len(memory_lines), 2)
        self.assert_rejected(valid.replace(memory_lines[0] + "\n", "", 1))
        self.assert_rejected(
            valid.replace(memory_lines[0], memory_lines[0] + "\n" + memory_lines[0], 1)
        )
        self.assert_rejected(valid.replace("schema=2", "schema=3", 1))
        obsolete_actor_key = "validLevel" + "Actors"
        self.assert_rejected(
            valid.replace(
                " commandLog=",
                f" {obsolete_actor_key}=406 commandLog=",
                1,
            )
        )
        self.assert_rejected(valid.replace("entityReused=1", "entityReused=0", 1))
        self.assert_rejected(
            valid.replace("destructionOverflow=0", "destructionOverflow=1", 1)
        )
        self.assert_rejected(
            valid.replace("commandLogAllocatedBytes=65536", "commandLogAllocatedBytes=1", 1)
        )
        lines = valid.splitlines()
        heartbeat_index = next(
            index
            for index, line in enumerate(lines)
            if "[ECHOES_STRESS_SUSTAINED_HEARTBEAT]" in line and "tick=200 " in line
        )
        memory_index = next(
            index
            for index, line in enumerate(lines)
            if "[ECHOES_STRESS_SUSTAINED_MEMORY]" in line and "tick=200 " in line
        )
        lines[heartbeat_index], lines[memory_index] = (
            lines[memory_index],
            lines[heartbeat_index],
        )
        self.assert_rejected("\n".join(lines) + "\n")

    def test_memory_telemetry_accepts_signed_natural_gc_deltas(self) -> None:
        valid = valid_log()
        valid = valid.replace("gcCycles=0", "gcCycles=1")
        valid = valid.replace(
            "gcLastPreUsedPhysicalBytes=0 gcLastPostUsedPhysicalBytes=0 "
            "gcLastUsedPhysicalDeltaBytes=0 gcLastObjectSlotDelta=0 "
            "gcLastClaimedObjectSlotDelta=0",
            "gcLastPreUsedPhysicalBytes=550000000 "
            "gcLastPostUsedPhysicalBytes=530000000 "
            "gcLastUsedPhysicalDeltaBytes=-20000000 gcLastObjectSlotDelta=-200 "
            "gcLastClaimedObjectSlotDelta=-400",
        )
        self.assertTrue(validate_log(valid, 10)["accepted"])
        self.assert_rejected(
            valid.replace(
                "gcLastUsedPhysicalDeltaBytes=-20000000",
                "gcLastUsedPhysicalDeltaBytes=-19999999",
            )
        )
        positive = valid.replace(
            "gcLastPreUsedPhysicalBytes=550000000 "
            "gcLastPostUsedPhysicalBytes=530000000 "
            "gcLastUsedPhysicalDeltaBytes=-20000000",
            "gcLastPreUsedPhysicalBytes=530000000 "
            "gcLastPostUsedPhysicalBytes=550000000 "
            "gcLastUsedPhysicalDeltaBytes=20000000",
        )
        self.assertTrue(validate_log(positive, 10)["accepted"])
        self.assert_rejected(
            positive.replace(
                "gcLastUsedPhysicalDeltaBytes=20000000",
                "gcLastUsedPhysicalDeltaBytes=19999999",
            )
        )

    def test_memory_telemetry_rejects_non_natural_or_forced_gc_claims(self) -> None:
        valid = valid_log()
        self.assert_rejected(valid.replace("naturalGc=true", "naturalGc=false", 1))
        self.assert_rejected(valid.replace("forcedGc=false", "forcedGc=true", 1))

    def test_post_warmup_pool_and_mid_plateau_is_required(self) -> None:
        with self.assertRaisesRegex(
            ValidationError,
            "post-warmup destruction pool or MID count did not plateau",
        ):
            validate_log(
                valid_log(130, replacement_seconds=(5, 125)),
                130,
            )

    def test_readiness_is_exact_and_precedes_heartbeats(self) -> None:
        valid = valid_log()
        first_heartbeat = next(
            line
            for line in valid.splitlines()
            if "[ECHOES_STRESS_SUSTAINED_HEARTBEAT]" in line
        )
        self.assert_rejected(valid.replace(READY + "\n", ""))
        self.assert_rejected(valid.replace(READY, READY + "\n" + READY))
        self.assert_rejected(first_heartbeat + "\n" + valid)

    def test_stabilization_contract_is_exact_and_precedes_readiness(self) -> None:
        valid = valid_log()
        self.assert_rejected(valid.replace(STABILIZED + "\n", ""))
        self.assert_rejected(
            valid.replace(STABILIZED, STABILIZED + "\n" + STABILIZED)
        )
        self.assert_rejected(valid.replace("stableFrames=20", "stableFrames=19", 1))
        self.assert_rejected(
            valid.replace("stableWallUs=1000000", "stableWallUs=999999", 1)
        )
        self.assert_rejected(
            valid.replace("minimumStableFrames=20 ", "", 1)
        )
        self.assert_rejected(
            valid.replace("stableWallUs=1000000", "stableWallUs=6000000", 1)
        )
        self.assert_rejected(
            valid.replace(
                "stableFrames=20 stableWallUs=1000000",
                "stableFrames=21 stableWallUs=1300000",
                1,
            )
        )
        self.assert_rejected(
            valid.replace(STABILIZED + "\n" + READY, READY + "\n" + STABILIZED)
        )

    def test_startup_reset_contract_is_exact_and_precedes_stabilization(self) -> None:
        valid = valid_log()
        self.assert_rejected(valid.replace("rawDeltaUs=300000", "rawDeltaUs=250000", 1))
        self.assert_rejected(
            valid.replace("stableFramesBeforeReset=7 ", "", 1)
        )
        self.assert_rejected(
            valid.replace(
                "stableFramesBeforeReset=7 stableWallUsBeforeReset=350000",
                "stableFramesBeforeReset=1 stableWallUsBeforeReset=900000",
                1,
            )
        )
        self.assert_rejected(
            valid.replace(
                "stableFramesBeforeReset=7 stableWallUsBeforeReset=350000",
                "stableFramesBeforeReset=20 stableWallUsBeforeReset=1000000",
                1,
            )
        )
        self.assert_rejected(
            valid.replace(
                STABILIZATION_RESET + "\n" + STABILIZED,
                STABILIZED + "\n" + STABILIZATION_RESET,
            )
        )

    def test_forbidden_runtime_markers_are_rejected(self) -> None:
        for marker in (
            "[ECHOES_STRESS_SUSTAINED_FAILED] code=TEST tick=20 detail=x",
            "[ECHOES_MATCH_FINISHED] outcome=won",
            "[ECHOES_MATCH_PAUSE] paused=true",
            "[ECHOES_SIM_TIME_CLAMP] discarded=true",
            "[ECHOES_SIM_VIEW_SYNC_FAILED]",
            "Fatal error:",
            "Assertion failed:",
            "GPU Crashed",
            "Out of memory",
            "LowLevelFatalError",
            "OUT OF VIDEO MEMORY",
            "[ECHOES_STRESS_READY] units=400",
            "[ECHOES_NEW_RUNTIME_FAILED] code=UNKNOWN",
            "Ensure condition failed: false",
            "Unhandled Exception: test",
            "SIGABRT",
            "SIGBUS",
        ):
            with self.subTest(marker=marker):
                self.assert_rejected(valid_log() + marker + "\n")

    def test_population_faction_and_view_drift_are_rejected(self) -> None:
        for old, new in (
            ("activePlayers=4", "activePlayers=3"),
            ("meridian=200", "meridian=199"),
            ("combatUnits=396", "combatUnits=400"),
            ("ownedEntities=400", "ownedEntities=399"),
            ("views=401", "views=400"),
        ):
            with self.subTest(field=old):
                self.assert_rejected(valid_log().replace(old, new, 1))

    def test_malformed_or_duplicate_fields_are_rejected(self) -> None:
        valid = valid_log()
        self.assert_rejected(valid.replace(" tick=20 ", " ", 1))
        self.assert_rejected(valid.replace(" tick=20 ", " tick=20 tick=20 ", 1))
        self.assert_rejected(valid.replace(" tick=20 ", " tick=twenty ", 1))
        self.assert_rejected(valid.replace(" tick=20 ", " tick=20 stray ", 1))
        self.assert_rejected(valid.replace(READY, "arbitrary-prefix " + READY, 1))
        self.assert_rejected(valid.replace(READY, READY + " " + READY, 1))

    def test_known_unreal_log_prefix_is_accepted(self) -> None:
        prefixed = "\n".join(
            f"[2026.08.31-03.13.09:112][  0]LogEchoes: Display: {line}"
            for line in valid_log().splitlines()
        )
        result = validate_log(prefixed + "\n", 10)
        self.assertTrue(result["accepted"])

    def test_tick_and_wall_cadence_fail_closed(self) -> None:
        valid = valid_log()
        self.assert_rejected(valid.replace("tick=40 wall_ms=2000", "tick=20 wall_ms=2000"))
        self.assert_rejected(valid.replace("tick=40 wall_ms=2000", "tick=60 wall_ms=2000"))
        self.assert_rejected(valid.replace("tick=40 wall_ms=2000", "tick=40 wall_ms=1000"))
        self.assert_rejected(valid.replace("tick=40 wall_ms=2000", "tick=40 wall_ms=7001"))

    def test_duration_requires_both_simulated_and_wall_time(self) -> None:
        self.assert_rejected(valid_log(), duration=11)
        self.assert_rejected(valid_log().replace("wall_ms=10000", "wall_ms=4000"))

    def test_activity_and_replacement_evidence_are_required(self) -> None:
        valid = valid_log()
        self.assert_rejected(valid.replace("activityAgeTicks=0", "activityAgeTicks=101", 1))
        self.assert_rejected(valid.replace("activeAttackMove=300", "activeAttackMove=0", 1))
        self.assert_rejected(valid.replace("intervalDamage=25", "intervalDamage=0"))
        no_replacements = (
            valid.replace("intervalCombatLosses=1", "intervalCombatLosses=0")
            .replace("cumulativeCombatLosses=1", "cumulativeCombatLosses=0")
            .replace("intervalReplacements=1", "intervalReplacements=0")
            .replace("cumulativeReplacements=1", "cumulativeReplacements=0")
            .replace("commandLog=398", "commandLog=397")
        )
        self.assert_rejected(no_replacements)

    def test_loss_replacement_and_command_accounting_are_exact(self) -> None:
        valid = valid_log()
        self.assert_rejected(valid.replace("intervalCombatLosses=1", "intervalCombatLosses=2"))
        self.assert_rejected(valid.replace("cumulativeReplacements=1", "cumulativeReplacements=2", 1))
        self.assert_rejected(valid.replace("commandLog=397", "commandLog=999", 1))
        self.assert_rejected(valid.replace("commandCapacity=262144", "commandCapacity=397", 1))
        self.assert_rejected(valid.replace("intervalOrderRenewals=1", "intervalOrderRenewals=5"))
        self.assert_rejected(valid.replace("activeAttackMove=300", "activeAttackMove=397", 1))
        self.assert_rejected(valid.replace("damagedCombatants=12", "damagedCombatants=397", 1))
        self.assert_rejected(valid.replace("checksum=10040", "checksum=10020", 1))

    def test_one_hour_boundary_cannot_race_or_forge_qualification(self) -> None:
        self.assert_rejected(valid_log(3600, qualified=False), duration=3600)
        self.assert_rejected(valid_log(3599, qualified=False), duration=3600)
        qualified = valid_log(3600, qualified=True)
        self.assert_rejected(qualified.replace("tick=72000 checksum=82000", "tick=72000 checksum=82001", 1))
        marker = qualified.splitlines()[-1]
        self.assert_rejected(qualified + marker + "\n", duration=3600)
        without_marker = qualified.splitlines()[:-1]
        ready_index = without_marker.index(READY)
        raced = "\n".join(
            without_marker[: ready_index + 1]
            + [marker]
            + without_marker[ready_index + 1 :]
        ) + "\n"
        self.assert_rejected(raced, duration=3600)


if __name__ == "__main__":
    unittest.main(verbosity=2)
