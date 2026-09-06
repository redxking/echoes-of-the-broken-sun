#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

/** Runtime provenance supplied by the command-dispatch path. Each observation
 * still has to resolve to the exact local command and SimCore receipt.
 */
enum class EEchoesTutorialOrderCommandOrigin : uint8
{
    DirectPlayerCommand,
    ContextAction
};

struct FEchoesTutorialExpectedCommand
{
    echoes::sim::CommandType Type = echoes::sim::CommandType::Stop;
    echoes::sim::EntityId Actor = 0;
    echoes::sim::EntityId Target = 0;
    echoes::sim::Vec2 Position{};
};

/** Actor and geometry bindings come from the authored training operation. */
struct FEchoesTutorialRouteSetup
{
    echoes::sim::PlayerId LocalPlayer = 0;
    /** First controller-local physical input-attempt ID eligible for this lesson. */
    std::uint64_t FirstInputAttemptSequence = 0;
    echoes::sim::EntityId MoveActor = 0;
    echoes::sim::Vec2 MoveDestination{};
    std::int32_t MoveInputToleranceRaw = 0;
    std::int32_t MoveArrivalToleranceRaw = 0;
    echoes::sim::EntityId StopActor = 0;
    echoes::sim::EntityId PatrolActor = 0;
    echoes::sim::Vec2 PatrolDestination{};
    std::int32_t PatrolInputToleranceRaw = 0;
    echoes::sim::EntityId GuardActor = 0;
    echoes::sim::EntityId GuardTarget = 0;
    FEchoesTutorialExpectedCommand ContextAction{};
    FEchoesTutorialExpectedCommand RejectedAction{};
    std::int32_t RejectedInputToleranceRaw = 0;
    echoes::sim::CommandResolutionOutcome ExpectedRejectionOutcome =
        echoes::sim::CommandResolutionOutcome::DestinationOccupied;
};

struct FEchoesTutorialReserveSetup
{
    static constexpr std::uint64_t RequiredDeliveredMatter = 200;

    echoes::sim::PlayerId LocalPlayer = 0;
    echoes::sim::EntityId Worker = 0;
    echoes::sim::EntityId MatterNode = 0;
};

/** Read-only lesson components used to present the next concrete Route action. */
struct FEchoesTutorialRouteProgress
{
    bool bMoveArrived = false;
    bool bContextActionCompleted = false;
    bool bStopCompleted = false;
    bool bPatrolCompleted = false;
    bool bGuardCompleted = false;
    bool bRejectionAcknowledged = false;

    [[nodiscard]] bool PredicateSatisfied() const
    {
        return bMoveArrived && bContextActionCompleted && bStopCompleted &&
            bPatrolCompleted && bGuardCompleted && bRejectionAcknowledged;
    }
};

/** Read-only Reserve evidence; delivered Matter is the bound worker's credit. */
struct FEchoesTutorialReserveProgress
{
    bool bGatherStarted = false;
    bool bDeliverObserved = false;
    bool bContinuousRoute = false;
    std::uint64_t DeliveredMatter = 0;

    [[nodiscard]] bool PredicateSatisfied() const
    {
        return bGatherStarted && bDeliverObserved && bContinuousRoute &&
            DeliveredMatter >=
                FEchoesTutorialReserveSetup::RequiredDeliveredMatter;
    }
};

/**
 * Source predicate for SPEC-LSN-004/005. It reads immutable current SimCore
 * state and exact accepted local-player sequence IDs. It does not write a
 * profile, publish narrative signals, or grant lesson/mastery progression.
 *
 * SnapshotToken is the runtime authority-generation token. A restore/retry
 * must start a new observation; mismatched tokens and nonconsecutive ticks
 * invalidate the current attempt rather than inventing missing evidence.
 */
class ECHOESOFTHEBROKENSUN_API FEchoesTutorialOrderObservation
{
public:
    bool BeginRoute(
        uint64 Session,
        uint64 SnapshotToken,
        const echoes::sim::Simulation& Simulation,
        const FEchoesTutorialRouteSetup& Setup);
    bool BeginReserve(
        uint64 Session,
        uint64 SnapshotToken,
        const echoes::sim::Simulation& Simulation,
        const FEchoesTutorialReserveSetup& Setup);

    void ObserveState(
        uint64 Session,
        uint64 SnapshotToken,
        const echoes::sim::Simulation& Simulation);
    void ObserveAcceptedCommand(
        uint64 Session,
        uint64 SnapshotToken,
        const echoes::sim::Simulation& Simulation,
        std::uint64_t Sequence,
        EEchoesTutorialOrderCommandOrigin Origin);
    bool ObserveRejectedAttempt(
        uint64 Session,
        uint64 SnapshotToken,
        const echoes::sim::Simulation& Simulation,
        std::uint64_t InputAttemptSequence,
        const FEchoesTutorialExpectedCommand& Attempt);
    void ObserveRejectionAcknowledged(
        uint64 Session,
        uint64 SnapshotToken,
        const echoes::sim::Simulation& Simulation,
        std::uint64_t InputAttemptSequence);

    void Reset();
    [[nodiscard]] bool IsActive() const;
    [[nodiscard]] FEchoesTutorialRouteProgress RouteProgress() const;
    [[nodiscard]] FEchoesTutorialReserveProgress ReserveProgress() const;
    [[nodiscard]] bool RouteSimulationPredicateSatisfied() const;
    [[nodiscard]] bool ReserveSimulationPredicateSatisfied() const;
    [[nodiscard]] std::uint64_t DeliveredMatterObserved() const
    {
        return DeliveredMatter;
    }

private:
    enum class EMode : uint8
    {
        None,
        Route,
        Reserve
    };

    bool AdvanceState(
        uint64 Session,
        uint64 SnapshotToken,
        const echoes::sim::Simulation& Simulation);

    EMode Mode = EMode::None;
    uint64 ActiveSession = 0;
    uint64 ActiveSnapshotToken = 0;
    echoes::sim::Tick LessonStartTick = 0;
    echoes::sim::Tick LastObservedTick = 0;
    std::uint64_t FirstAllowedSequence = 0;
    FEchoesTutorialRouteSetup RouteSetup{};
    FEchoesTutorialReserveSetup ReserveSetup{};
    TSet<std::uint64_t> ProcessedSequences;
    TSet<std::uint64_t> ProcessedInputAttempts;
    std::uint64_t PendingRejectedAttemptSequence = 0;

    echoes::sim::Vec2 ObservedMoveDestination{};
    echoes::sim::Order PreviousMoveOrder{};
    echoes::sim::Order PreviousStopOrder{};
    echoes::sim::Order StopOrderBeforeAdvance{};
    echoes::sim::Tick PreviousStateTick = 0;
    bool bMoveApplied = false;
    bool bMoveOrderObserved = false;
    bool bMoveArrivalObserved = false;
    bool bContextActionObserved = false;
    bool bStopObserved = false;
    bool bPatrolObserved = false;
    bool bGuardObserved = false;
    bool bRejectionAcknowledged = false;

    bool bGatherApplied = false;
    bool bGatherOrderObserved = false;
    bool bDeliverOrderObserved = false;
    bool bContinuousReserveRoute = false;
    std::uint64_t DeliveredMatter = 0;
};
