// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesResourceMonitorModel.h"

namespace
{
using echoes::sim::Entity;
using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::HarvestState;
using echoes::sim::OrderType;
using echoes::sim::PlayerView;
using echoes::sim::ProductionQueueItem;
using echoes::sim::ProducerQueueState;
using echoes::sim::Tick;

int32 SaturatingSum(int32 Existing, int32 Addition)
{
    return static_cast<int32>(FMath::Clamp<int64>(
        static_cast<int64>(Existing) + static_cast<int64>(Addition),
        MIN_int32, MAX_int32));
}

uint64 RemainingTicks(Tick DueTick, Tick ObservedTick)
{
    return DueTick > ObservedTick ? DueTick - ObservedTick : 0;
}

void AddWorkerActivity(const Entity& Worker, FEchoesResourceMonitorWorkers& Out)
{
    ++Out.Total;
    switch (Worker.harvestState)
    {
        case HarvestState::MovingToResource:
        case HarvestState::ReturningHome:
            ++Out.TravelingResourceRoute;
            return;
        case HarvestState::Harvesting:
            ++Out.Gathering;
            return;
        case HarvestState::Delivering:
            ++Out.Delivering;
            return;
        case HarvestState::Idle:
            break;
    }

    switch (Worker.order.type)
    {
        case OrderType::None:
            ++Out.Idle;
            return;
        case OrderType::Build:
            ++Out.Building;
            return;
        case OrderType::Repair:
            ++Out.Repairing;
            return;
        default:
            ++Out.OtherAssigned;
            return;
    }
}

void AddAssignment(
    TMap<uint32, int32>& BySource,
    const Entity& Worker)
{
    if (Worker.assignedResourceNode != 0)
    {
        BySource.FindOrAdd(Worker.assignedResourceNode)++;
    }
}

FEchoesResourceMonitorInvestedCommitment MakeActiveCommitment(
    const ProducerQueueState& Queue)
{
    FEchoesResourceMonitorInvestedCommitment Out;
    Out.OwnerEntityId = Queue.producer;
    Out.ProductionItemId = Queue.activeItem.itemId;
    Out.ItemType = Queue.activeItem.unitType;
    Out.InvestedMatter = Queue.activeItem.investedCost.material;
    Out.InvestedDawn = Queue.activeItem.investedCost.dawnshards;
    Out.ReservedLogistics = Queue.activeItem.logisticsCost;
    Out.Progress = Queue.activeProgress;
    Out.RequiredProgress = Queue.activeItem.requiredTicks;
    return Out;
}

FEchoesResourceMonitorWaitingRequest MakeWaitingRequest(
    EntityId Producer,
    const ProductionQueueItem& Item)
{
    FEchoesResourceMonitorWaitingRequest Out;
    Out.ProducerEntityId = Producer;
    Out.ProductionItemId = Item.itemId;
    Out.ItemType = Item.unitType;
    Out.RequestedMatter = Item.configuredCost.material;
    Out.RequestedDawn = Item.configuredCost.dawnshards;
    Out.RequestedLogistics = Item.logisticsCost;
    Out.RequiredTicks = Item.requiredTicks;
    return Out;
}

void AddOwnedTimers(
    const PlayerView& PlayerView,
    const Entity& EntityValue,
    FEchoesResourceMonitorView& Out)
{
    const Tick TickNow = PlayerView.CurrentTick();
    if (EntityValue.relaySupplyActive &&
        EntityValue.relaySupplyUntilTick > TickNow)
    {
        Out.RelayExpiries.Add({EntityValue.id, EntityValue.relaySupplyUntilTick,
            RemainingTicks(EntityValue.relaySupplyUntilTick, TickNow)});
    }

    if (EntityValue.type == EntityType::FutureWell &&
        EntityValue.wellChoice == echoes::sim::FutureWellChoice::Preserve &&
        EntityValue.wellActivationTick != 0 &&
        PlayerView.Config().rules.futureWell.preserveIntervalTicks > 0)
    {
        // Step credits before incrementing currentTick. PlayerView observes the
        // completed tick, so the next credit opportunity is activation + interval.
        const Tick Interval = PlayerView.Config().rules.futureWell.preserveIntervalTicks;
        Tick DueTick = EntityValue.wellActivationTick + Interval;
        if (DueTick <= TickNow)
        {
            const Tick ElapsedIntervals = (TickNow - DueTick) / Interval + 1;
            DueTick += ElapsedIntervals * Interval;
        }
        Out.PreserveReturns.Add({EntityValue.id, DueTick,
            RemainingTicks(DueTick, TickNow)});
    }

    if (EntityValue.choirCoherenceNextChargeTick > TickNow)
    {
        Out.ChoirCoherenceCharges.Add({EntityValue.id,
            EntityValue.choirCoherenceNextChargeTick,
            RemainingTicks(EntityValue.choirCoherenceNextChargeTick, TickNow)});
    }
}

void AddConstructionCommitment(
    const Entity& EntityValue,
    FEchoesResourceMonitorView& Out)
{
    if (EntityValue.completed || EntityValue.constructionRequired <= 0)
    {
        return;
    }
    FEchoesResourceMonitorInvestedCommitment Commitment;
    Commitment.OwnerEntityId = EntityValue.id;
    Commitment.ItemType = EntityValue.type;
    Commitment.InvestedMatter = EntityValue.constructionInvestedCost.material;
    Commitment.InvestedDawn = EntityValue.constructionInvestedCost.dawnshards;
    Commitment.Progress = EntityValue.constructionProgress;
    Commitment.RequiredProgress = EntityValue.constructionRequired;
    Commitment.bConstruction = true;
    Out.ActiveInvestedCommitments.Add(Commitment);
}
} // namespace

FEchoesResourceMonitorView FEchoesResourceMonitorModel::Build(
    const PlayerView& PlayerView)
{
    FEchoesResourceMonitorView Out;
    Out.Recipient = PlayerView.Player().id;
    Out.ObservedTick = PlayerView.CurrentTick();
    Out.LiquidFunds.Matter = PlayerView.Player().resources.material;
    Out.LiquidFunds.Dawn = PlayerView.Player().resources.dawnshards;
    Out.Logistics.Used = PlayerView.PopulationUsed();
    Out.Logistics.Capacity = PlayerView.PopulationCapacity();
    Out.Workers.bActivityAvailable = true;
    Out.Availability.bWorkerSourceAssignmentsAvailable = true;
    Out.Availability.bRelayExpiryAvailable = true;
    Out.Availability.bPreserveReturnAvailable = true;
    Out.Availability.bChoirCoherenceTimersAvailable = true;

    TMap<uint32, int32> Assignments;
    TSet<uint32> ConnectedRelays;
    for (const EntityId RelayId : PlayerView.ConnectedRelayUnits())
    {
        ConnectedRelays.Add(RelayId);
    }

    for (const Entity& EntityValue : PlayerView.Entities())
    {
        if (EntityValue.owner != Out.Recipient)
        {
            continue;
        }
        if (EntityValue.type == EntityType::Worker && EntityValue.hitPoints > 0)
        {
            AddWorkerActivity(EntityValue, Out.Workers);
            AddAssignment(Assignments, EntityValue);
        }
        AddConstructionCommitment(EntityValue, Out);
        AddOwnedTimers(PlayerView, EntityValue, Out);
        if (ConnectedRelays.Contains(EntityValue.id) &&
            EntityValue.relaySupplyActive &&
            EntityValue.relaySupplyUntilTick > Out.ObservedTick)
        {
            Out.Logistics.TemporaryCapacityFromActiveRelays = SaturatingSum(
                Out.Logistics.TemporaryCapacityFromActiveRelays,
                PlayerView.Config().rules.relaySupply.capacityBonus);
        }
    }
    Out.Logistics.bTemporaryCapacityAvailable = true;

    for (const TPair<uint32, int32>& Assignment : Assignments)
    {
        Out.MatterSourceAssignments.Add(
            {Assignment.Key, Assignment.Value});
    }
    Out.MatterSourceAssignments.Sort(
        [](const FEchoesResourceMonitorSourceAssignment& Left,
           const FEchoesResourceMonitorSourceAssignment& Right)
        {
            return Left.ResourceEntityId < Right.ResourceEntityId;
        });

    for (const ProducerQueueState& Queue : PlayerView.ProducerQueues())
    {
        if (Queue.active)
        {
            const FEchoesResourceMonitorInvestedCommitment Active =
                MakeActiveCommitment(Queue);
            Out.Logistics.ActiveReserved = SaturatingSum(
                Out.Logistics.ActiveReserved, Active.ReservedLogistics);
            Out.ActiveInvestedCommitments.Add(Active);
        }
        for (const ProductionQueueItem& Waiting : Queue.waiting)
        {
            const FEchoesResourceMonitorWaitingRequest Request =
                MakeWaitingRequest(Queue.producer, Waiting);
            Out.Logistics.WaitingRequested = SaturatingSum(
                Out.Logistics.WaitingRequested, Request.RequestedLogistics);
            Out.WaitingUninvestedRequests.Add(Request);
        }
    }

    // Deposit quantity in a PlayerView is intentionally presence-only for
    // non-owned nodes; no aggregate or forecast may be derived here.
    // Queue admission does not expose a contract that reserves waiting slots.
    return Out;
}

FEchoesResourceMonitorView FEchoesResourceMonitorModel::BuildNetworkScoped(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe)
{
    FEchoesResourceMonitorView Out;
    Out.Recipient = Keyframe.player;
    Out.ObservedTick = Keyframe.simulationTick;
    Out.bNetworkScoped = true;
    Out.LiquidFunds.Matter = Keyframe.resources.material;
    Out.LiquidFunds.Dawn = Keyframe.resources.dawnshards;
    Out.Logistics.Used = Keyframe.populationUsed;
    Out.Logistics.Capacity = Keyframe.populationCapacity;

    // Scoped network packets carry entity identity/type but omit worker state,
    // queues, costs, relay state, and Future Well lifecycle fields.
    for (const echoes::sim::net::ScopedEntityState& EntityValue : Keyframe.entities)
    {
        if (EntityValue.owner == Keyframe.player &&
            EntityValue.type == EntityType::Worker && EntityValue.hitPoints > 0)
        {
            ++Out.Workers.Total;
        }
    }
    return Out;
}
