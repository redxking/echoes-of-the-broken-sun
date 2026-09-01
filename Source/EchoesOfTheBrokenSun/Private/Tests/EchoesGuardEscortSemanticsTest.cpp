#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesContentSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "HAL/PlatformTime.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

namespace EchoesGuardEscortSemantics
{
// Characterization constants. These mirror the private simulation constants
// kGuardLeashRaw and kGuardFollowRaw; a change to either shows up here as an
// explicit test diff rather than a silent behavior shift.
constexpr std::int32_t ExpectedGuardLeashRaw = 6 * echoes::sim::kFixedScale;
constexpr std::int32_t ExpectedGuardFollowRaw = 2 * echoes::sim::kFixedScale;
constexpr std::uint32_t ContentTicksPerSecond = 20;
constexpr std::int64_t ScenarioTickBudget = 600;
constexpr double SuiteWallClockBudgetSeconds = 30.0;

struct FCommandIssuer final
{
    echoes::sim::Simulation* Simulation = nullptr;
    std::uint64_t NextSequence[2] = {1, 1};

    std::optional<echoes::sim::CommandResolutionOutcome> IssueAndStep(
        FAutomationTestBase& Test,
        const TCHAR* Label,
        echoes::sim::Command Command)
    {
        Command.executeTick = Simulation->CurrentTick();
        Command.sequence = NextSequence[Command.player]++;
        std::string RejectionReason;
        if (!Simulation->QueueCommand(Command, &RejectionReason))
        {
            Test.AddError(FString::Printf(
                TEXT("%s: structural queue rejection: %s"),
                Label,
                UTF8_TO_TCHAR(RejectionReason.c_str())));
            return std::nullopt;
        }
        Simulation->Step();
        const std::optional<echoes::sim::CommandResolutionReceipt> Receipt =
            Simulation->FindCommandResolutionReceipt(
                Command.player, Command.sequence);
        if (!Receipt.has_value())
        {
            Test.AddError(FString::Printf(
                TEXT("%s: no resolution receipt after execution step"), Label));
            return std::nullopt;
        }
        return Receipt->outcome;
    }
};

// The pace-gap scenario runs along one vertical line, so the raw gap is the
// absolute y-distance and stays exact in integer math.
[[nodiscard]] std::int64_t VerticalGapRaw(
    const echoes::sim::Simulation& Simulation,
    echoes::sim::EntityId Leader,
    echoes::sim::EntityId Follower)
{
    const echoes::sim::Entity* LeaderEntity = Simulation.FindEntity(Leader);
    const echoes::sim::Entity* FollowerEntity = Simulation.FindEntity(Follower);
    if (LeaderEntity == nullptr || FollowerEntity == nullptr)
    {
        return -1;
    }
    const std::int64_t Delta =
        static_cast<std::int64_t>(LeaderEntity->position.y.Raw()) -
        FollowerEntity->position.y.Raw();
    return Delta >= 0 ? Delta : -Delta;
}

[[nodiscard]] bool OrderIs(
    const echoes::sim::Simulation& Simulation,
    echoes::sim::EntityId Actor,
    echoes::sim::OrderType Type,
    echoes::sim::EntityId Target)
{
    const echoes::sim::Entity* Entity = Simulation.FindEntity(Actor);
    return Entity != nullptr && Entity->order.type == Type &&
           Entity->order.target == Target;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesGuardEscortSemanticsTest,
    "Echoes.Runtime.AI.GuardEscortSemantics",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesGuardEscortSemanticsTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace EchoesGuardEscortSemantics;
    namespace sim = echoes::sim;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const double SuiteStartSeconds = FPlatformTime::Seconds();
    const auto WallClockExceeded = [&]() -> bool
    {
        if (FPlatformTime::Seconds() - SuiteStartSeconds >
            SuiteWallClockBudgetSeconds)
        {
            AddError(TEXT("Suite wall-clock budget exhausted"));
            return true;
        }
        return false;
    };

    FEchoesContentCatalog Catalog;
    FString ContentError;
    if (!FEchoesContentCatalog::LoadCanonicalPack(
            UEchoesContentSubsystem::GetCanonicalPackPath(),
            UEchoesContentSubsystem::GetCanonicalDigestPath(),
            Catalog,
            ContentError))
    {
        AddError(FString::Printf(
            TEXT("Canonical content pack unavailable: %s"), *ContentError));
        return false;
    }
    sim::SimulationRules Rules;
    FString RulesError;
    if (!Catalog.BuildSimulationRules(ContentTicksPerSecond, Rules, RulesError))
    {
        AddError(FString::Printf(
            TEXT("Simulation rules unavailable: %s"), *RulesError));
        return false;
    }

    // Characterize the content-derived stats every scenario below depends on.
    // raw = cm/s * 1024 / (20 ticks/s * 100). A rules-data retune is expected
    // to change these lines and only these lines.
    const sim::EntityArchetypeRules& MeridianScout =
        Rules.archetypes[0][static_cast<std::size_t>(sim::EntityType::ScoutUnit)];
    const sim::EntityArchetypeRules& MeridianSoldier =
        Rules.archetypes[0][static_cast<std::size_t>(sim::EntityType::Soldier)];
    const sim::EntityArchetypeRules& MeridianHeavy =
        Rules.archetypes[0][static_cast<std::size_t>(sim::EntityType::HeavyUnit)];
    const sim::EntityArchetypeRules& MeridianWorker =
        Rules.archetypes[0][static_cast<std::size_t>(sim::EntityType::Worker)];
    TestEqual(TEXT("Relay Skiff movement is 256 raw/tick (500 cm/s)"),
              MeridianScout.movementPerTickRaw, 256);
    TestEqual(TEXT("Lancer movement is 163 raw/tick (320 cm/s)"),
              MeridianSoldier.movementPerTickRaw, 163);
    TestEqual(TEXT("Bulwark movement is 117 raw/tick (230 cm/s)"),
              MeridianHeavy.movementPerTickRaw, 117);
    TestEqual(TEXT("Lancer attack range is 6656 raw (650 cm)"),
              MeridianSoldier.attackRangeRaw, 6656);
    TestEqual(TEXT("Workers carry no attack and therefore cannot Hold or Guard"),
              MeridianWorker.attackDamage, 0);
    TestTrue(TEXT("Lancer range plus both unit footprints exceeds the 6-tile Guard response scan"),
             MeridianSoldier.attackRangeRaw +
                     2 * MeridianScout.footprintHalfExtentRaw >
                 ExpectedGuardLeashRaw);

    sim::SimulationConfig BaseConfig;
    BaseConfig.rules = Rules;
    BaseConfig.randomSeed = 1;

    // Scenario 1 — escort pace gap. A Lancer guarding a moving Relay Skiff
    // falls behind at exactly the speed difference per tick; the gap crosses
    // the 6-tile response leash at the closed-form tick and the guard only
    // reforms after the guarded unit stops.
    {
        sim::Simulation Simulation(BaseConfig);
        TestTrue(TEXT("S1: local player joins"),
                 Simulation.AddPlayer(0, sim::Faction::MeridianCompact, {0, 0}));
        const sim::EntityId Vip = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::ScoutUnit,
            sim::Vec2::FromTiles(10, 12));
        const sim::EntityId Guard = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(10, 10));
        TestTrue(TEXT("S1: fixture spawned"), Vip != 0 && Guard != 0);
        if (Vip == 0 || Guard == 0)
        {
            return false;
        }

        FCommandIssuer Issuer{&Simulation};
        sim::Command GuardCommand{};
        GuardCommand.player = 0;
        GuardCommand.type = sim::CommandType::Guard;
        GuardCommand.actor = Guard;
        GuardCommand.target = Vip;
        TestTrue(TEXT("S1: Guard(Skiff) admitted for a Lancer"),
                 Issuer.IssueAndStep(*this, TEXT("S1 guard"), GuardCommand) ==
                     sim::CommandResolutionOutcome::Applied);
        TestTrue(TEXT("S1: Lancer order is Guard(Skiff)"),
                 OrderIs(Simulation, Guard, sim::OrderType::Guard, Vip));
        TestEqual(TEXT("S1: stationary reformed gap sits exactly on the follow radius"),
                  VerticalGapRaw(Simulation, Vip, Guard),
                  static_cast<std::int64_t>(ExpectedGuardFollowRaw));

        const std::int32_t SpeedDelta = MeridianScout.movementPerTickRaw -
                                        MeridianSoldier.movementPerTickRaw;
        TestTrue(TEXT("S1: guarded unit is strictly faster than its escort"),
                 SpeedDelta > 0);
        const std::int64_t PredictedBreakTick =
            (ExpectedGuardLeashRaw - ExpectedGuardFollowRaw) / SpeedDelta + 1;

        sim::Command MoveCommand{};
        MoveCommand.player = 0;
        MoveCommand.type = sim::CommandType::Move;
        MoveCommand.actor = Vip;
        MoveCommand.position = sim::Vec2::FromTiles(10, 52);
        TestTrue(TEXT("S1: 40-tile Move admitted"),
                 Issuer.IssueAndStep(*this, TEXT("S1 move"), MoveCommand) ==
                     sim::CommandResolutionOutcome::Applied);

        // The execution step above was also the first movement tick.
        std::int64_t MovementTicks = 1;
        std::int64_t PreviousGap = VerticalGapRaw(Simulation, Vip, Guard);
        std::int64_t ObservedBreakTick = 0;
        std::int64_t MaximumGap = PreviousGap;
        bool SteadyDeltaHeld = true;
        if (PreviousGap > ExpectedGuardLeashRaw)
        {
            ObservedBreakTick = MovementTicks;
        }
        while (MovementTicks < ScenarioTickBudget && !WallClockExceeded())
        {
            const sim::Entity* VipEntity = Simulation.FindEntity(Vip);
            if (VipEntity == nullptr)
            {
                AddError(TEXT("S1: guarded unit vanished during an empty-map leg"));
                break;
            }
            if (VipEntity->order.type == sim::OrderType::None)
            {
                break;
            }
            Simulation.Step();
            ++MovementTicks;
            const std::int64_t Gap = VerticalGapRaw(Simulation, Vip, Guard);
            const sim::Entity* VipAfter = Simulation.FindEntity(Vip);
            const bool bVipStillMoving =
                VipAfter != nullptr &&
                VipAfter->order.type == sim::OrderType::Move;
            if (bVipStillMoving && Gap - PreviousGap != SpeedDelta)
            {
                SteadyDeltaHeld = false;
            }
            if (ObservedBreakTick == 0 && Gap > ExpectedGuardLeashRaw)
            {
                ObservedBreakTick = MovementTicks;
            }
            MaximumGap = std::max(MaximumGap, Gap);
            PreviousGap = Gap;
        }
        TestTrue(TEXT("S1: gap grows by exactly the speed delta on every full movement tick"),
                 SteadyDeltaHeld);
        TestEqual(TEXT("S1: gap crosses the 6-tile leash at the closed-form tick"),
                  ObservedBreakTick, PredictedBreakTick);
        TestTrue(TEXT("S1: transit gap exceeds the leash — the escort cannot protect in transit"),
                 MaximumGap > ExpectedGuardLeashRaw);

        std::int64_t ReformTicks = 0;
        while (ReformTicks < ScenarioTickBudget && !WallClockExceeded() &&
               VerticalGapRaw(Simulation, Vip, Guard) >
                   ExpectedGuardFollowRaw)
        {
            Simulation.Step();
            ++ReformTicks;
        }
        TestTrue(TEXT("S1: escort reforms to the follow radius only after the guarded unit stops"),
                 VerticalGapRaw(Simulation, Vip, Guard) <=
                     ExpectedGuardFollowRaw);
        TestTrue(TEXT("S1: Guard order survives the whole leg"),
                 OrderIs(Simulation, Guard, sim::OrderType::Guard, Vip));
    }

    // Scenario 2 — response envelope, clear-on-death, and idle passivity. An
    // enemy Lancer besieges a guarded Skiff from 6.5 tiles: outside the
    // guarded-centered 6-tile scan, inside its own weapon reach. The guards
    // never respond, the Guard orders clear in the very step the guarded unit
    // is culled, and the survivors stay inert even under point-blank fire.
    {
        sim::Simulation Simulation(BaseConfig);
        TestTrue(TEXT("S2: local player joins"),
                 Simulation.AddPlayer(0, sim::Faction::MeridianCompact, {0, 0}));
        TestTrue(TEXT("S2: opposing player joins"),
                 Simulation.AddPlayer(1, sim::Faction::MeridianCompact, {0, 0}));
        const sim::Vec2 VipPosition = sim::Vec2::FromTiles(20, 20);
        const sim::EntityId Vip = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::ScoutUnit,
            VipPosition);
        const sim::EntityId GuardWest = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(18, 20));
        const sim::EntityId GuardSouth = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(20, 22));
        // Exactly 6.5 tiles east of the guarded unit: 6656 raw > the 6144-raw
        // scan, yet within 6656 + 2*128 attack reach.
        const sim::EntityId Foe = Simulation.SpawnEntity(
            1, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromRaw(VipPosition.x.Raw() + 6656, VipPosition.y.Raw()));
        TestTrue(TEXT("S2: fixture spawned"),
                 Vip != 0 && GuardWest != 0 && GuardSouth != 0 && Foe != 0);
        if (Vip == 0 || GuardWest == 0 || GuardSouth == 0 || Foe == 0)
        {
            return false;
        }

        FCommandIssuer Issuer{&Simulation};
        sim::Command Command{};
        Command.player = 0;
        Command.type = sim::CommandType::Guard;
        Command.actor = GuardWest;
        Command.target = Vip;
        TestTrue(TEXT("S2: west Guard admitted"),
                 Issuer.IssueAndStep(*this, TEXT("S2 guard west"), Command) ==
                     sim::CommandResolutionOutcome::Applied);
        Command.actor = GuardSouth;
        TestTrue(TEXT("S2: south Guard admitted"),
                 Issuer.IssueAndStep(*this, TEXT("S2 guard south"), Command) ==
                     sim::CommandResolutionOutcome::Applied);
        sim::Command FoeHold{};
        FoeHold.player = 1;
        FoeHold.type = sim::CommandType::Hold;
        FoeHold.actor = Foe;
        TestTrue(TEXT("S2: besieging Hold admitted"),
                 Issuer.IssueAndStep(*this, TEXT("S2 foe hold"), FoeHold) ==
                     sim::CommandResolutionOutcome::Applied);

        const std::int32_t FoeMaxHitPoints =
            Simulation.FindEntity(Foe)->hitPoints;
        const sim::Vec2 GuardWestPosition =
            Simulation.FindEntity(GuardWest)->position;
        const sim::Vec2 GuardSouthPosition =
            Simulation.FindEntity(GuardSouth)->position;
        bool GuardsHeldDuringSiege = true;
        bool FoeUntouchedDuringSiege = true;
        std::int64_t SiegeTicks = 0;
        while (Simulation.FindEntity(Vip) != nullptr &&
               SiegeTicks < ScenarioTickBudget && !WallClockExceeded())
        {
            Simulation.Step();
            ++SiegeTicks;
            if (Simulation.FindEntity(Vip) == nullptr)
            {
                break;
            }
            const sim::Entity* West = Simulation.FindEntity(GuardWest);
            const sim::Entity* South = Simulation.FindEntity(GuardSouth);
            const sim::Entity* FoeEntity = Simulation.FindEntity(Foe);
            if (West == nullptr || South == nullptr || FoeEntity == nullptr)
            {
                AddError(TEXT("S2: unexpected casualty during the siege window"));
                break;
            }
            if (!OrderIs(Simulation, GuardWest, sim::OrderType::Guard, Vip) ||
                !OrderIs(Simulation, GuardSouth, sim::OrderType::Guard, Vip) ||
                !(West->position == GuardWestPosition) ||
                !(South->position == GuardSouthPosition))
            {
                GuardsHeldDuringSiege = false;
            }
            if (FoeEntity->hitPoints != FoeMaxHitPoints)
            {
                FoeUntouchedDuringSiege = false;
            }
        }
        TestTrue(TEXT("S2: guarded unit dies to fire from outside the 6-tile response scan"),
                 Simulation.FindEntity(Vip) == nullptr);
        TestTrue(TEXT("S2: both guards keep Guard orders and never move or engage during the siege"),
                 GuardsHeldDuringSiege);
        TestTrue(TEXT("S2: the besieger is never fired upon"),
                 FoeUntouchedDuringSiege);
        TestTrue(TEXT("S2: west Guard order clears in the culling step itself"),
                 OrderIs(Simulation, GuardWest, sim::OrderType::None, 0));
        TestTrue(TEXT("S2: south Guard order clears in the culling step itself"),
                 OrderIs(Simulation, GuardSouth, sim::OrderType::None, 0));

        // Walk the besieger point-blank and reopen fire: the surviving former
        // guards have no auto-acquire and never respond.
        sim::Command FoeMove{};
        FoeMove.player = 1;
        FoeMove.type = sim::CommandType::Move;
        FoeMove.actor = Foe;
        FoeMove.position = sim::Vec2::FromTiles(21, 20);
        TestTrue(TEXT("S2: approach Move admitted"),
                 Issuer.IssueAndStep(*this, TEXT("S2 foe move"), FoeMove) ==
                     sim::CommandResolutionOutcome::Applied);
        std::int64_t ApproachTicks = 0;
        while (ApproachTicks < ScenarioTickBudget && !WallClockExceeded() &&
               Simulation.FindEntity(Foe) != nullptr &&
               Simulation.FindEntity(Foe)->order.type != sim::OrderType::None)
        {
            Simulation.Step();
            ++ApproachTicks;
        }
        TestTrue(TEXT("S2: reopened Hold admitted"),
                 Issuer.IssueAndStep(*this, TEXT("S2 foe hold 2"), FoeHold) ==
                     sim::CommandResolutionOutcome::Applied);
        const std::int32_t GuardSouthHitPointsBefore =
            Simulation.FindEntity(GuardSouth)->hitPoints;
        bool SurvivorsStayedPassive = true;
        for (std::int32_t WindowTick = 0;
             WindowTick < 40 && !WallClockExceeded();
             ++WindowTick)
        {
            Simulation.Step();
            const sim::Entity* West = Simulation.FindEntity(GuardWest);
            const sim::Entity* South = Simulation.FindEntity(GuardSouth);
            if ((West != nullptr &&
                 West->order.type != sim::OrderType::None) ||
                (South != nullptr &&
                 South->order.type != sim::OrderType::None))
            {
                SurvivorsStayedPassive = false;
            }
        }
        const sim::Entity* GuardSouthAfter = Simulation.FindEntity(GuardSouth);
        TestTrue(TEXT("S2: a former guard takes point-blank fire after the loss"),
                 GuardSouthAfter == nullptr ||
                     GuardSouthAfter->hitPoints < GuardSouthHitPointsBefore);
        TestTrue(TEXT("S2: order-less survivors never retaliate or move"),
                 SurvivorsStayedPassive);
        TestTrue(TEXT("S2: the besieger is still untouched at the end"),
                 Simulation.FindEntity(Foe) != nullptr &&
                     Simulation.FindEntity(Foe)->hitPoints == FoeMaxHitPoints);
    }

    // Scenario 3 — Hold and Guard admission. Workers (no attack) can be guard
    // targets but can neither Hold nor Guard; attack-capable units can.
    {
        sim::Simulation Simulation(BaseConfig);
        TestTrue(TEXT("S3: local player joins"),
                 Simulation.AddPlayer(0, sim::Faction::MeridianCompact, {0, 0}));
        const sim::EntityId Worker = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Worker,
            sim::Vec2::FromTiles(30, 30));
        const sim::EntityId Skiff = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::ScoutUnit,
            sim::Vec2::FromTiles(32, 30));
        const sim::EntityId Lancer = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(30, 32));
        TestTrue(TEXT("S3: fixture spawned"),
                 Worker != 0 && Skiff != 0 && Lancer != 0);
        if (Worker == 0 || Skiff == 0 || Lancer == 0)
        {
            return false;
        }

        FCommandIssuer Issuer{&Simulation};
        sim::Command Command{};
        Command.player = 0;
        Command.type = sim::CommandType::Hold;
        Command.actor = Worker;
        TestTrue(TEXT("S3: worker Hold resolves NoEffect — workers cannot Hold"),
                 Issuer.IssueAndStep(*this, TEXT("S3 worker hold"), Command) ==
                     sim::CommandResolutionOutcome::NoEffect);
        TestTrue(TEXT("S3: worker order stays None after the refused Hold"),
                 OrderIs(Simulation, Worker, sim::OrderType::None, 0));

        Command.actor = Skiff;
        TestTrue(TEXT("S3: Skiff Hold resolves Applied — Talar-class units can Hold"),
                 Issuer.IssueAndStep(*this, TEXT("S3 skiff hold"), Command) ==
                     sim::CommandResolutionOutcome::Applied);
        TestTrue(TEXT("S3: Skiff order is Hold"),
                 OrderIs(Simulation, Skiff, sim::OrderType::Hold, 0));

        Command.type = sim::CommandType::Guard;
        Command.actor = Worker;
        Command.target = Skiff;
        TestTrue(TEXT("S3: worker Guard resolves NoEffect — workers cannot Guard"),
                 Issuer.IssueAndStep(*this, TEXT("S3 worker guard"), Command) ==
                     sim::CommandResolutionOutcome::NoEffect);

        Command.actor = Lancer;
        Command.target = Worker;
        TestTrue(TEXT("S3: Guard with a worker as the guarded target is Applied"),
                 Issuer.IssueAndStep(*this, TEXT("S3 guard worker"), Command) ==
                     sim::CommandResolutionOutcome::Applied);
        TestTrue(TEXT("S3: Lancer order is Guard(worker)"),
                 OrderIs(Simulation, Lancer, sim::OrderType::Guard, Worker));

        Command.target = Lancer;
        TestTrue(TEXT("S3: self-Guard resolves NoEffect"),
                 Issuer.IssueAndStep(*this, TEXT("S3 self guard"), Command) ==
                     sim::CommandResolutionOutcome::NoEffect);
    }

    // Scenario 4 — guarding an incomplete structure is admitted, and the
    // Guard order clears in the same step the structure is destroyed.
    {
        sim::Simulation Simulation(BaseConfig);
        TestTrue(TEXT("S4: local player joins"),
                 Simulation.AddPlayer(0, sim::Faction::MeridianCompact, {0, 0}));
        const sim::EntityId Relay = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::UtilityStructure,
            sim::Vec2::FromTiles(40, 40));
        const sim::EntityId Lancer = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(42, 40));
        TestTrue(TEXT("S4: fixture spawned"), Relay != 0 && Lancer != 0);
        if (Relay == 0 || Lancer == 0)
        {
            return false;
        }
        sim::Entity* RelayEntity = Simulation.MutableEntityForTesting(Relay);
        if (RelayEntity == nullptr)
        {
            AddError(TEXT("S4: relay fixture unavailable"));
            return false;
        }
        RelayEntity->completed = false;
        RelayEntity->constructionProgress = 10;

        FCommandIssuer Issuer{&Simulation};
        sim::Command Command{};
        Command.player = 0;
        Command.type = sim::CommandType::Guard;
        Command.actor = Lancer;
        Command.target = Relay;
        TestTrue(TEXT("S4: Guard of an incomplete structure is Applied"),
                 Issuer.IssueAndStep(*this, TEXT("S4 guard relay"), Command) ==
                     sim::CommandResolutionOutcome::Applied);
        TestTrue(TEXT("S4: Lancer order is Guard(incomplete relay)"),
                 OrderIs(Simulation, Lancer, sim::OrderType::Guard, Relay));

        Simulation.Step();
        TestTrue(TEXT("S4: Guard order persists while the incomplete structure lives"),
                 OrderIs(Simulation, Lancer, sim::OrderType::Guard, Relay));

        sim::Entity* RelayToDestroy = Simulation.MutableEntityForTesting(Relay);
        if (RelayToDestroy == nullptr)
        {
            AddError(TEXT("S4: relay vanished before the destruction step"));
            return false;
        }
        RelayToDestroy->hitPoints = 0;
        Simulation.Step();
        TestTrue(TEXT("S4: destroyed structure is culled"),
                 Simulation.FindEntity(Relay) == nullptr);
        TestTrue(TEXT("S4: Guard order clears in the culling step itself"),
                 OrderIs(Simulation, Lancer, sim::OrderType::None, 0));
    }

    return true;
}

#endif
