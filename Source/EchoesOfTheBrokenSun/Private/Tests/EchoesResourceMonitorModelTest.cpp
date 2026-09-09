// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesResourceMonitorModel.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesResourceMonitorModelTest,
    "Echoes.Runtime.Presentation.ResourceMonitor.PlayerScopedFacts",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesResourceMonitorModelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace echoes::sim;

    SimulationConfig Config;
    Config.mapWidthTiles = 16;
    Config.mapHeightTiles = 16;
    Simulation SimulationValue(Config);
    TestTrue(TEXT("Monitor player is admitted"),
        SimulationValue.AddPlayer(0, Faction::MeridianCompact, {320, 70}));
    TestTrue(TEXT("Other recipient is admitted"),
        SimulationValue.AddPlayer(1, Faction::KharuunAssemblies, {999, 999}));

    const EntityId IdleWorker = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(2, 2));
    const EntityId GatheringWorker = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(3, 2));
    const EntityId DeliveringWorker = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(4, 2));
    const EntityId Builder = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(5, 2));
    const EntityId Producer = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks, Vec2::FromTiles(2, 4));
    const EntityId Construction = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(4, 4));
    const EntityId Relay = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::ScoutUnit, Vec2::FromTiles(6, 2));
    // SpawnEntity refuses ResourceNode and FutureWell by contract; a Well is
    // created through its own route and starts neutral, so the recipient claims
    // it below before the monitor can scope a Preserve return to them.
    const EntityId PreserveWell =
        SimulationValue.SpawnFutureWell(Vec2::FromTiles(6, 4));
    const EntityId ChoirStructure = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::UtilityStructure, Vec2::FromTiles(7, 4));
    const EntityId HiddenEnemyWorker = SimulationValue.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Worker, Vec2::FromTiles(14, 14));
    TestTrue(TEXT("Scoped monitor fixture exists"),
        IdleWorker != 0 && GatheringWorker != 0 && DeliveringWorker != 0 &&
            Builder != 0 && Producer != 0 && Construction != 0 && Relay != 0 &&
            PreserveWell != 0 && ChoirStructure != 0 && HiddenEnemyWorker != 0);

    if (Entity* Worker = SimulationValue.MutableEntityForTesting(GatheringWorker))
    {
        Worker->harvestState = HarvestState::Harvesting;
        Worker->assignedResourceNode = 501;
    }
    if (Entity* Worker = SimulationValue.MutableEntityForTesting(DeliveringWorker))
    {
        Worker->harvestState = HarvestState::Delivering;
        Worker->assignedResourceNode = 501;
    }
    if (Entity* Worker = SimulationValue.MutableEntityForTesting(Builder))
    {
        Worker->order.type = OrderType::Build;
    }
    if (Entity* Building = SimulationValue.MutableEntityForTesting(Construction))
    {
        Building->completed = false;
        Building->constructionProgress = 5;
        Building->constructionRequired = 20;
        Building->constructionInvestedCost = {40, 6};
    }
    if (Entity* Building = SimulationValue.MutableEntityForTesting(Producer))
    {
        Building->activeProductionItemId = 7001;
        Building->productionType = EntityType::Worker;
        Building->productionProgress = 8;
        Building->productionRequired = 30;
        Building->productionInvestedCost = {60, 12};
        Building->productionLogisticsCost = 1;
        ProductionQueueItem Waiting;
        Waiting.itemId = 7002;
        Waiting.unitType = EntityType::Soldier;
        Waiting.configuredCost = {85, 20};
        Waiting.requiredTicks = 100;
        Waiting.logisticsCost = 2;
        Building->productionQueue.push_back(Waiting);
    }
    if (Entity* EntityValue = SimulationValue.MutableEntityForTesting(Relay))
    {
        EntityValue->relaySupplyActive = true;
        EntityValue->relaySupplyUntilTick = 40;
    }
    if (Entity* EntityValue = SimulationValue.MutableEntityForTesting(PreserveWell))
    {
        EntityValue->owner = 0;
        EntityValue->wellChoice = FutureWellChoice::Preserve;
        EntityValue->wellActivationTick = 1;
    }
    if (Entity* EntityValue = SimulationValue.MutableEntityForTesting(ChoirStructure))
    {
        EntityValue->choirCoherenceNextChargeTick = 90;
    }

    const std::optional<PlayerView> PlayerView = SimulationValue.CreatePlayerView(0);
    if (!TestTrue(TEXT("Player view is materialized"), PlayerView.has_value()))
    {
        return false;
    }
    const FEchoesResourceMonitorView Monitor =
        FEchoesResourceMonitorModel::Build(*PlayerView);

    TestTrue(TEXT("Liquid funds are exact player-scoped values"),
        Monitor.Recipient == 0 && Monitor.LiquidFunds.Matter == 320 &&
            Monitor.LiquidFunds.Dawn == 70 && !Monitor.bNetworkScoped);
    TestTrue(TEXT("Worker phases and known assignments are read from owned PlayerView workers"),
        Monitor.Workers.bActivityAvailable && Monitor.Workers.Total == 4 &&
            Monitor.Workers.Idle == 1 && Monitor.Workers.Gathering == 1 &&
            Monitor.Workers.Delivering == 1 && Monitor.Workers.Building == 1 &&
            Monitor.MatterSourceAssignments.Num() == 1 &&
            Monitor.MatterSourceAssignments[0].ResourceEntityId == 501 &&
            Monitor.MatterSourceAssignments[0].AssignedWorkers == 2);
    TestTrue(TEXT("Hidden or unowned workers cannot alter recipient totals"),
        Monitor.Workers.Total == 4 &&
            !Monitor.MatterSourceAssignments.ContainsByPredicate(
                [HiddenEnemyWorker](const FEchoesResourceMonitorSourceAssignment& Assignment)
                {
                    return Assignment.ResourceEntityId == HiddenEnemyWorker;
                }));
    TestTrue(TEXT("Active investment is separate from an uninvested waiting request"),
        Monitor.ActiveInvestedCommitments.ContainsByPredicate(
            [Producer](const FEchoesResourceMonitorInvestedCommitment& Commitment)
            {
                return !Commitment.bConstruction && Commitment.OwnerEntityId == Producer &&
                    Commitment.ProductionItemId == 7001 &&
                    Commitment.InvestedMatter == 60 && Commitment.InvestedDawn == 12 &&
                    Commitment.ReservedLogistics == 1;
            }) &&
        Monitor.WaitingUninvestedRequests.Num() == 1 &&
            Monitor.WaitingUninvestedRequests[0].ProductionItemId == 7002 &&
            Monitor.WaitingUninvestedRequests[0].RequestedMatter == 85 &&
            Monitor.WaitingUninvestedRequests[0].RequestedDawn == 20 &&
            Monitor.Logistics.ActiveReserved == 1 &&
            Monitor.Logistics.WaitingRequested == 2 &&
            !Monitor.Logistics.bWaitingRequestsAreReserved);
    TestTrue(TEXT("Owned construction keeps its already invested cost distinct"),
        Monitor.ActiveInvestedCommitments.ContainsByPredicate(
            [Construction](const FEchoesResourceMonitorInvestedCommitment& Commitment)
            {
                return Commitment.bConstruction && Commitment.OwnerEntityId == Construction &&
                    Commitment.InvestedMatter == 40 && Commitment.InvestedDawn == 6 &&
                    Commitment.Progress == 5 && Commitment.RequiredProgress == 20;
            }));
    TestTrue(TEXT("Only exact owned source-scoped timers are published"),
        Monitor.Availability.bRelayExpiryAvailable &&
            Monitor.Availability.bPreserveReturnAvailable &&
            Monitor.Availability.bChoirCoherenceTimersAvailable &&
            Monitor.RelayExpiries.ContainsByPredicate(
                [Relay](const FEchoesResourceMonitorTimer& Timer)
                { return Timer.SourceEntityId == Relay && Timer.DueTick == 40; }) &&
            Monitor.PreserveReturns.ContainsByPredicate(
                [PreserveWell](const FEchoesResourceMonitorTimer& Timer)
                { return Timer.SourceEntityId == PreserveWell && Timer.DueTick == 301; }) &&
            Monitor.ChoirCoherenceCharges.ContainsByPredicate(
                [ChoirStructure](const FEchoesResourceMonitorTimer& Timer)
                { return Timer.SourceEntityId == ChoirStructure && Timer.DueTick == 90; }));
    TestTrue(TEXT("Uncarried income, deposit, blockage, and route-time facts stay unavailable"),
        !Monitor.Availability.bRealizedIncomeAvailable &&
            !Monitor.Availability.bMatterDepositAmountsAvailable &&
            !Monitor.Availability.bDepletionProjectionAvailable &&
            !Monitor.Availability.bBlockedWorkerCountAvailable &&
            !Monitor.Availability.bRouteTimeAvailable &&
            !Monitor.Logistics.bPermanentCapacityAvailable);

    // Check the displayed tick against an actual credited step, not another
    // copy of the monitor formula. Use a clean fixture with no other economy.
    Simulation TimerSimulation(Config);
    TimerSimulation.AddPlayer(0, Faction::MeridianCompact, {0, 0});
    const EntityId TimerWell =
        TimerSimulation.SpawnFutureWell(Vec2::FromTiles(6, 4));
    if (Entity* Well = TimerSimulation.MutableEntityForTesting(TimerWell))
    {
        Well->owner = 0;
        Well->wellChoice = FutureWellChoice::Preserve;
        Well->wellActivationTick = 1;
    }
    TimerSimulation.Step(Config.rules.futureWell.preserveIntervalTicks);
    const auto BeforeCredit = TimerSimulation.CreatePlayerView(0);
    if (TestTrue(TEXT("Pre-credit scoped view exists"), BeforeCredit.has_value()))
    {
        const auto Before = FEchoesResourceMonitorModel::Build(*BeforeCredit);
        TestTrue(TEXT("Preserve reports one tick until the actual credited step"),
            Before.PreserveReturns.Num() == 1 &&
            Before.PreserveReturns[0].RemainingTicks == 1 &&
            Before.LiquidFunds.Dawn == 0);
        TimerSimulation.Step();
        const auto AfterCredit = TimerSimulation.CreatePlayerView(0);
        if (TestTrue(TEXT("Post-credit scoped view exists"), AfterCredit.has_value()))
        {
            const auto After = FEchoesResourceMonitorModel::Build(*AfterCredit);
            TestTrue(TEXT("Actual credit advances the monitor to the next full interval"),
                After.LiquidFunds.Dawn == Config.rules.futureWell.preserveDawnPerInterval &&
                After.PreserveReturns.Num() == 1 &&
                After.PreserveReturns[0].RemainingTicks == Config.rules.futureWell.preserveIntervalTicks);
        }
    }

    echoes::sim::net::ScopedViewKeyframe Keyframe;
    Keyframe.player = 1;
    Keyframe.simulationTick = 55;
    Keyframe.resources = {77, 13};
    Keyframe.populationUsed = 3;
    Keyframe.populationCapacity = 9;
    Keyframe.entities.push_back({41, 1, Faction::KharuunAssemblies, EntityType::Worker});
    Keyframe.entities.push_back({42, 0, Faction::MeridianCompact, EntityType::Worker});
    const FEchoesResourceMonitorView Network =
        FEchoesResourceMonitorModel::BuildNetworkScoped(Keyframe);
    TestTrue(TEXT("Network monitor uses only packet-carried recipient funds and logistics"),
        Network.bNetworkScoped && Network.Recipient == 1 && Network.ObservedTick == 55 &&
            Network.LiquidFunds.Matter == 77 && Network.LiquidFunds.Dawn == 13 &&
            Network.Logistics.Used == 3 && Network.Logistics.Capacity == 9 &&
            Network.Workers.Total == 1);
    TestTrue(TEXT("Network monitor does not invent omitted activity or timers"),
        !Network.Workers.bActivityAvailable &&
            !Network.Availability.bWorkerSourceAssignmentsAvailable &&
            !Network.Availability.bRelayExpiryAvailable &&
            !Network.Availability.bPreserveReturnAvailable &&
            !Network.Availability.bChoirCoherenceTimersAvailable &&
            Network.ActiveInvestedCommitments.IsEmpty() &&
            Network.WaitingUninvestedRequests.IsEmpty());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
