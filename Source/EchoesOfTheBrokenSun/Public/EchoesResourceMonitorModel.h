// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis
#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimCore/Simulation.h"

/**
 * Read-only, recipient-scoped economy facts for REL-ECO-017 presentation.
 * Values are intentionally typed rather than formatted so the shell can make
 * its own accessibility and tactical-layout decisions.
 */
struct FEchoesResourceMonitorFunds final
{
    int32 Matter = 0;
    int32 Dawn = 0;
};

/** Exact current worker phases. A false availability flag means no zero is implied. */
struct FEchoesResourceMonitorWorkers final
{
    int32 Total = 0;
    int32 Idle = 0;
    int32 Gathering = 0;
    int32 Delivering = 0;
    int32 TravelingResourceRoute = 0;
    int32 Building = 0;
    int32 Repairing = 0;
    int32 OtherAssigned = 0;
    bool bActivityAvailable = false;
    bool bBlockedAvailable = false;
    bool bRouteTimeAvailable = false;
};

/** An owned worker-to-resource-node association; this is not a deposit amount. */
struct FEchoesResourceMonitorSourceAssignment final
{
    uint32 ResourceEntityId = 0;
    int32 AssignedWorkers = 0;
};

/** Cost already invested in an active owned production or construction item. */
struct FEchoesResourceMonitorInvestedCommitment final
{
    uint32 OwnerEntityId = 0;
    uint64 ProductionItemId = 0;
    echoes::sim::EntityType ItemType = echoes::sim::EntityType::Worker;
    int32 InvestedMatter = 0;
    int32 InvestedDawn = 0;
    int32 ReservedLogistics = 0;
    int32 Progress = 0;
    int32 RequiredProgress = 0;
    bool bConstruction = false;
};

/** A queued request remains uninvested by the simulation contract. */
struct FEchoesResourceMonitorWaitingRequest final
{
    uint32 ProducerEntityId = 0;
    uint64 ProductionItemId = 0;
    echoes::sim::EntityType ItemType = echoes::sim::EntityType::Worker;
    int32 RequestedMatter = 0;
    int32 RequestedDawn = 0;
    int32 RequestedLogistics = 0;
    int32 RequiredTicks = 0;
};

/** A source-scoped timer carried by an owned Relay, Preserve Well, or Choir structure. */
struct FEchoesResourceMonitorTimer final
{
    uint32 SourceEntityId = 0;
    uint64 DueTick = 0;
    uint64 RemainingTicks = 0;
};

/** Logistics facts with availability boundaries for unexposed classifications. */
struct FEchoesResourceMonitorLogistics final
{
    int32 Used = 0;
    int32 Capacity = 0;
    int32 ActiveReserved = 0;
    int32 WaitingRequested = 0;
    int32 TemporaryCapacityFromActiveRelays = 0;
    bool bPermanentCapacityAvailable = false;
    bool bTemporaryCapacityAvailable = false;
    bool bWaitingRequestsAreReserved = false;
};

/** Facts not carried by the current scoped input are explicitly unavailable. */
struct FEchoesResourceMonitorAvailability final
{
    bool bRealizedIncomeAvailable = false;
    bool bMatterDepositAmountsAvailable = false;
    bool bDepletionProjectionAvailable = false;
    bool bWorkerSourceAssignmentsAvailable = false;
    bool bOperationalDropoffsAvailable = false;
    bool bBlockedWorkerCountAvailable = false;
    bool bRouteTimeAvailable = false;
    bool bRelayExpiryAvailable = false;
    bool bPreserveReturnAvailable = false;
    bool bChoirCoherenceTimersAvailable = false;
};

/** Complete player-facing monitor snapshot, built without an authoritative Simulation reference. */
struct FEchoesResourceMonitorView final
{
    echoes::sim::PlayerId Recipient = echoes::sim::kNeutralPlayer;
    uint64 ObservedTick = 0;
    bool bNetworkScoped = false;
    FEchoesResourceMonitorFunds LiquidFunds;
    FEchoesResourceMonitorWorkers Workers;
    FEchoesResourceMonitorLogistics Logistics;
    FEchoesResourceMonitorAvailability Availability;
    TArray<FEchoesResourceMonitorSourceAssignment> MatterSourceAssignments;
    TArray<FEchoesResourceMonitorInvestedCommitment> ActiveInvestedCommitments;
    TArray<FEchoesResourceMonitorWaitingRequest> WaitingUninvestedRequests;
    TArray<FEchoesResourceMonitorTimer> RelayExpiries;
    TArray<FEchoesResourceMonitorTimer> PreserveReturns;
    TArray<FEchoesResourceMonitorTimer> ChoirCoherenceCharges;
};

/** REL-ECO-017 adapter. It consumes scoped observation only and never forecasts income. */
class ECHOESOFTHEBROKENSUN_API FEchoesResourceMonitorModel final
{
public:
    static FEchoesResourceMonitorView Build(const echoes::sim::PlayerView& PlayerView);

    /**
     * A network client receives this deliberately smaller projection. It may
     * show packet-carried funds, capacity, and owned worker count, but never
     * reaches into a local Simulation to fill omitted detail.
     */
    static FEchoesResourceMonitorView BuildNetworkScoped(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe);
};
