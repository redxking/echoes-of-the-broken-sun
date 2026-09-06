#include "EchoesTutorialOrderObservation.h"

#include <limits>

namespace
{
namespace sim = echoes::sim;

const sim::Entity* FindViewEntity(
    const sim::PlayerView& View,
    sim::EntityId Id)
{
    for (const sim::Entity& Entity : View.Entities())
        if (Entity.id == Id) return &Entity;
    return nullptr;
}

bool IsLivingOwnedActor(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    sim::EntityId Id)
{
    const sim::Entity* Entity = Simulation.FindEntity(Id);
    return Entity != nullptr && Entity->owner == Player && Entity->completed &&
        Entity->hitPoints > 0;
}

bool IsContextActionType(sim::CommandType Type)
{
    return Type == sim::CommandType::Gather ||
        Type == sim::CommandType::Attack;
}

bool UsesPosition(sim::CommandType Type)
{
    return Type == sim::CommandType::Move ||
        Type == sim::CommandType::AttackMove ||
        Type == sim::CommandType::Patrol ||
        Type == sim::CommandType::RaiseMineralCover;
}

bool UsesTarget(sim::CommandType Type)
{
    return Type == sim::CommandType::Gather ||
        Type == sim::CommandType::Deliver ||
        Type == sim::CommandType::Attack ||
        Type == sim::CommandType::Guard ||
        Type == sim::CommandType::FutureWell ||
        Type == sim::CommandType::AdaptWarform;
}

bool CommandMatches(
    const sim::Command& Command,
    const FEchoesTutorialExpectedCommand& Expected)
{
    if (Command.type != Expected.Type || Command.actor != Expected.Actor)
        return false;
    if (UsesPosition(Command.type) && Command.position != Expected.Position)
        return false;
    if (UsesTarget(Command.type) && Command.target != Expected.Target)
        return false;
    return true;
}

const sim::Command* FindCommand(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    std::uint64_t Sequence)
{
    for (const sim::Command& Command : Simulation.CommandLog())
        if (Command.player == Player && Command.sequence == Sequence)
            return &Command;
    return nullptr;
}

bool HasAppliedCommandForActorAtTick(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    sim::EntityId Actor,
    sim::Tick Tick)
{
    for (const sim::Command& Command : Simulation.CommandLog())
    {
        if (Command.player != Player || Command.actor != Actor ||
            Command.executeTick != Tick)
            continue;
        const auto Receipt = Simulation.FindCommandResolutionReceipt(
            Player, Command.sequence);
        if (Receipt.has_value() &&
            Receipt->outcome == sim::CommandResolutionOutcome::Applied)
            return true;
    }
    return false;
}

bool IsAtDestination(
    sim::Vec2 Position,
    sim::Vec2 Destination,
    std::int32_t ToleranceRaw)
{
    const std::int64_t DeltaX =
        static_cast<std::int64_t>(Position.x.Raw()) - Destination.x.Raw();
    const std::int64_t DeltaY =
        static_cast<std::int64_t>(Position.y.Raw()) - Destination.y.Raw();
    const std::uint64_t AbsX = static_cast<std::uint64_t>(
        DeltaX < 0 ? -DeltaX : DeltaX);
    const std::uint64_t AbsY = static_cast<std::uint64_t>(
        DeltaY < 0 ? -DeltaY : DeltaY);
    const std::uint64_t Tolerance =
        static_cast<std::uint64_t>(ToleranceRaw);
    if (AbsX > Tolerance || AbsY > Tolerance) return false;
    return AbsX * AbsX + AbsY * AbsY <= Tolerance * Tolerance;
}

bool IsSameTerrainTile(sim::Vec2 First, sim::Vec2 Second)
{
    return First.x.FloorToInt() == Second.x.FloorToInt() &&
        First.y.FloorToInt() == Second.y.FloorToInt();
}

bool ContextOrderMatches(
    const sim::Entity& Actor,
    const FEchoesTutorialExpectedCommand& Expected)
{
    if (Expected.Type == sim::CommandType::Gather)
    {
        return Actor.assignedResourceNode == Expected.Target &&
            (Actor.order.type == sim::OrderType::Gather ||
                Actor.order.type == sim::OrderType::Deliver);
    }
    return Expected.Type == sim::CommandType::Attack &&
        Actor.order.type == sim::OrderType::Attack &&
        Actor.order.target == Expected.Target;
}
}

void FEchoesTutorialOrderObservation::Reset()
{
    *this = FEchoesTutorialOrderObservation{};
}

bool FEchoesTutorialOrderObservation::IsActive() const
{
    return Mode != EMode::None && ActiveSession != 0 &&
        ActiveSnapshotToken != 0;
}

bool FEchoesTutorialOrderObservation::BeginRoute(
    uint64 Session,
    uint64 SnapshotToken,
    const sim::Simulation& Simulation,
    const FEchoesTutorialRouteSetup& Setup)
{
    Reset();
    const auto FirstSequence = Simulation.NextCommandSequence(Setup.LocalPlayer);
    const sim::Entity* MoveActor = Simulation.FindEntity(Setup.MoveActor);
    const sim::Entity* PatrolActor = Simulation.FindEntity(Setup.PatrolActor);
    const sim::Entity* GuardActor = Simulation.FindEntity(Setup.GuardActor);
    const sim::Entity* GuardTarget = Simulation.FindEntity(Setup.GuardTarget);
    if (Session == 0 || SnapshotToken == 0 || !FirstSequence.has_value() ||
        Setup.MoveInputToleranceRaw < 0 ||
        Setup.MoveArrivalToleranceRaw < 0 ||
        Setup.PatrolInputToleranceRaw < 0 ||
        Setup.RejectedInputToleranceRaw < 0 ||
        !IsLivingOwnedActor(Simulation, Setup.LocalPlayer, Setup.MoveActor) ||
        !IsLivingOwnedActor(Simulation, Setup.LocalPlayer, Setup.StopActor) ||
        !IsLivingOwnedActor(Simulation, Setup.LocalPlayer, Setup.PatrolActor) ||
        !IsLivingOwnedActor(Simulation, Setup.LocalPlayer, Setup.GuardActor) ||
        !IsLivingOwnedActor(Simulation, Setup.LocalPlayer, Setup.GuardTarget) ||
        !IsLivingOwnedActor(
            Simulation, Setup.LocalPlayer, Setup.ContextAction.Actor) ||
        !IsLivingOwnedActor(
            Simulation, Setup.LocalPlayer, Setup.RejectedAction.Actor) ||
        MoveActor == nullptr || MoveActor->movementPerTickRaw <= 0 ||
        PatrolActor == nullptr || PatrolActor->attackDamage <= 0 ||
        GuardActor == nullptr || GuardActor->attackDamage <= 0 ||
        GuardTarget == nullptr || Setup.GuardActor == Setup.GuardTarget ||
        Setup.FirstInputAttemptSequence == 0 ||
        !IsContextActionType(Setup.ContextAction.Type) ||
        Setup.RejectedAction.Type != sim::CommandType::Move ||
        Simulation.ValidateMoveOrder(
            Setup.LocalPlayer, Setup.RejectedAction.Actor,
            Setup.RejectedAction.Position) !=
            Setup.ExpectedRejectionOutcome ||
        Setup.ExpectedRejectionOutcome ==
            sim::CommandResolutionOutcome::Applied ||
        Simulation.ValidateMoveOrder(
            Setup.LocalPlayer, Setup.MoveActor, Setup.MoveDestination) !=
            sim::CommandResolutionOutcome::Applied ||
        !Simulation.IsPositionPassable(Setup.PatrolDestination) ||
        PatrolActor->position == Setup.PatrolDestination)
        return false;

    const sim::Entity* ContextActor =
        Simulation.FindEntity(Setup.ContextAction.Actor);
    const sim::Entity* ContextTarget =
        Simulation.FindEntity(Setup.ContextAction.Target);
    if (ContextActor == nullptr || ContextTarget == nullptr ||
        (Setup.ContextAction.Type == sim::CommandType::Gather &&
            (ContextActor->type != sim::EntityType::Worker ||
                ContextTarget->type != sim::EntityType::ResourceNode ||
                ContextTarget->resourceRemaining <= 0 ||
                !Simulation.IsEntityVisibleTo(
                    Setup.LocalPlayer, ContextTarget->id))) ||
        (Setup.ContextAction.Type == sim::CommandType::Attack &&
            (ContextActor->attackDamage <= 0 ||
                !Simulation.Config().IsHostile(
                    Setup.LocalPlayer, ContextTarget->owner) ||
                !Simulation.IsEntityVisibleTo(
                    Setup.LocalPlayer, ContextTarget->id))))
        return false;

    RouteSetup = Setup;
    Mode = EMode::Route;
    ActiveSession = Session;
    ActiveSnapshotToken = SnapshotToken;
    LessonStartTick = Simulation.CurrentTick();
    LastObservedTick = LessonStartTick;
    PreviousStateTick = LessonStartTick;
    FirstAllowedSequence = *FirstSequence;
    PreviousMoveOrder = MoveActor->order;
    PreviousStopOrder = Simulation.FindEntity(Setup.StopActor)->order;
    return true;
}

bool FEchoesTutorialOrderObservation::BeginReserve(
    uint64 Session,
    uint64 SnapshotToken,
    const sim::Simulation& Simulation,
    const FEchoesTutorialReserveSetup& Setup)
{
    Reset();
    const auto FirstSequence = Simulation.NextCommandSequence(Setup.LocalPlayer);
    const sim::Entity* Worker = Simulation.FindEntity(Setup.Worker);
    const sim::Entity* MatterNode = Simulation.FindEntity(Setup.MatterNode);
    if (Session == 0 || SnapshotToken == 0 || !FirstSequence.has_value() ||
        !IsLivingOwnedActor(Simulation, Setup.LocalPlayer, Setup.Worker) ||
        Worker == nullptr || Worker->type != sim::EntityType::Worker ||
        Worker->cargoCapacity <= 0 || Worker->workRate <= 0 ||
        MatterNode == nullptr || MatterNode->type != sim::EntityType::ResourceNode ||
        MatterNode->hitPoints <= 0 || MatterNode->resourceRemaining <=
            static_cast<std::int32_t>(
                FEchoesTutorialReserveSetup::RequiredDeliveredMatter) ||
        !Simulation.IsEntityVisibleTo(Setup.LocalPlayer, Setup.MatterNode))
        return false;

    ReserveSetup = Setup;
    Mode = EMode::Reserve;
    ActiveSession = Session;
    ActiveSnapshotToken = SnapshotToken;
    LessonStartTick = Simulation.CurrentTick();
    LastObservedTick = LessonStartTick;
    PreviousStateTick = LessonStartTick;
    FirstAllowedSequence = *FirstSequence;
    return true;
}

bool FEchoesTutorialOrderObservation::AdvanceState(
    uint64 Session,
    uint64 SnapshotToken,
    const sim::Simulation& Simulation)
{
    if (!IsActive() || Session != ActiveSession) return false;
    if (SnapshotToken != ActiveSnapshotToken)
    {
        Reset();
        return false;
    }
    const auto View = Simulation.CreatePlayerView(
        Mode == EMode::Route ? RouteSetup.LocalPlayer : ReserveSetup.LocalPlayer);
    if (!View.has_value())
    {
        Reset();
        return false;
    }
    const sim::Tick CurrentTick = View->CurrentTick();
    if (CurrentTick < LessonStartTick || CurrentTick < LastObservedTick ||
        (CurrentTick > LastObservedTick &&
            CurrentTick - LastObservedTick != 1))
    {
        Reset();
        return false;
    }
    if (CurrentTick == LastObservedTick) return true;

    PreviousStateTick = LastObservedTick;
    if (Mode == EMode::Route)
    {
        StopOrderBeforeAdvance = PreviousStopOrder;
        const sim::Entity* MoveActor = FindViewEntity(*View, RouteSetup.MoveActor);
        const sim::Entity* StopActor = FindViewEntity(*View, RouteSetup.StopActor);
        const sim::Entity* PatrolActor = FindViewEntity(*View, RouteSetup.PatrolActor);
        const sim::Entity* GuardActor = FindViewEntity(*View, RouteSetup.GuardActor);
        const sim::Entity* GuardTarget = FindViewEntity(*View, RouteSetup.GuardTarget);
        if (MoveActor == nullptr || StopActor == nullptr || PatrolActor == nullptr ||
            GuardActor == nullptr || GuardTarget == nullptr ||
            MoveActor->hitPoints <= 0 || StopActor->hitPoints <= 0 ||
            PatrolActor->hitPoints <= 0 || GuardActor->hitPoints <= 0 ||
            GuardTarget->hitPoints <= 0)
        {
            Reset();
            return false;
        }
        if (bMoveApplied && MoveActor->order.type == sim::OrderType::Move &&
            MoveActor->order.destination == ObservedMoveDestination)
            bMoveOrderObserved = true;
        if (bMoveApplied && bMoveOrderObserved &&
            PreviousMoveOrder.type == sim::OrderType::Move &&
            PreviousMoveOrder.destination == ObservedMoveDestination &&
            MoveActor->order.type == sim::OrderType::None &&
            IsAtDestination(MoveActor->position, ObservedMoveDestination,
                RouteSetup.MoveArrivalToleranceRaw) &&
            !HasAppliedCommandForActorAtTick(
                Simulation, RouteSetup.LocalPlayer, RouteSetup.MoveActor,
                PreviousStateTick))
            bMoveArrivalObserved = true;
        PreviousMoveOrder = MoveActor->order;
        PreviousStopOrder = StopActor->order;
    }
    else
    {
        const sim::Entity* Worker = FindViewEntity(*View, ReserveSetup.Worker);
        if (Worker == nullptr || Worker->owner != ReserveSetup.LocalPlayer ||
            Worker->hitPoints <= 0 || !Worker->completed)
        {
            Reset();
            return false;
        }
        const bool bBoundGather =
            Worker->assignedResourceNode == ReserveSetup.MatterNode &&
            Worker->order.target == ReserveSetup.MatterNode &&
            Worker->order.type == sim::OrderType::Gather;
        const bool bBoundDeliver =
            Worker->assignedResourceNode == ReserveSetup.MatterNode &&
            Worker->order.type == sim::OrderType::Deliver;
        bGatherOrderObserved |= bGatherApplied && bBoundGather;
        bDeliverOrderObserved |= bGatherOrderObserved && bBoundDeliver;
        bContinuousReserveRoute = bBoundGather || bBoundDeliver;
        if (bGatherApplied && bGatherOrderObserved && bDeliverOrderObserved)
        {
            for (const sim::MaterialDeliveryReceipt& Receipt :
                 View->MaterialDeliveries())
            {
                if (Receipt.worker != ReserveSetup.Worker || Receipt.amount <= 0 ||
                    Receipt.tick == std::numeric_limits<sim::Tick>::max() ||
                    Receipt.tick + 1 != CurrentTick)
                    continue;
                const std::uint64_t Amount =
                    static_cast<std::uint64_t>(Receipt.amount);
                DeliveredMatter =
                    std::numeric_limits<std::uint64_t>::max() - DeliveredMatter < Amount
                        ? std::numeric_limits<std::uint64_t>::max()
                        : DeliveredMatter + Amount;
            }
        }
    }
    LastObservedTick = CurrentTick;
    return true;
}

void FEchoesTutorialOrderObservation::ObserveState(
    uint64 Session,
    uint64 SnapshotToken,
    const sim::Simulation& Simulation)
{
    (void)AdvanceState(Session, SnapshotToken, Simulation);
}

void FEchoesTutorialOrderObservation::ObserveAcceptedCommand(
    uint64 Session,
    uint64 SnapshotToken,
    const sim::Simulation& Simulation,
    std::uint64_t Sequence,
    EEchoesTutorialOrderCommandOrigin Origin)
{
    if (!AdvanceState(Session, SnapshotToken, Simulation) ||
        Sequence < FirstAllowedSequence || ProcessedSequences.Contains(Sequence))
        return;
    const sim::PlayerId Player =
        Mode == EMode::Route ? RouteSetup.LocalPlayer : ReserveSetup.LocalPlayer;
    const sim::Command* Command = FindCommand(Simulation, Player, Sequence);
    const auto Receipt = Simulation.FindCommandResolutionReceipt(Player, Sequence);
    if (Command == nullptr || !Receipt.has_value()) return;
    ProcessedSequences.Add(Sequence);
    if (Command->executeTick < LessonStartTick ||
        Receipt->player != Player || Receipt->commandType != Command->type ||
        Receipt->assignedExecutionTick != Command->executeTick)
        return;

    if (Mode == EMode::Reserve)
    {
        if (Origin != EEchoesTutorialOrderCommandOrigin::ContextAction ||
            Receipt->outcome != sim::CommandResolutionOutcome::Applied ||
            Command->type != sim::CommandType::Gather ||
            Command->actor != ReserveSetup.Worker ||
            Command->target != ReserveSetup.MatterNode)
            return;
        const auto View = Simulation.CreatePlayerView(Player);
        const sim::Entity* Worker =
            View.has_value() ? FindViewEntity(*View, ReserveSetup.Worker) : nullptr;
        if (Worker != nullptr &&
            Worker->assignedResourceNode == ReserveSetup.MatterNode &&
            (Worker->order.type == sim::OrderType::Gather ||
                Worker->order.type == sim::OrderType::Deliver))
        {
            bGatherApplied = true;
            bGatherOrderObserved |=
                Worker->order.type == sim::OrderType::Gather;
        }
        return;
    }

    if (Receipt->outcome != sim::CommandResolutionOutcome::Applied) return;

    const auto View = Simulation.CreatePlayerView(Player);
    if (!View.has_value()) return;
    const sim::Entity* Actor = FindViewEntity(*View, Command->actor);
    if (Actor == nullptr || Actor->hitPoints <= 0) return;
    const bool bDirect =
        Origin == EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand;
    const bool bContext =
        Origin == EEchoesTutorialOrderCommandOrigin::ContextAction;
    if (!bDirect && !bContext)
    {
        Reset();
        return;
    }
    if (bContext && CommandMatches(*Command, RouteSetup.ContextAction) &&
        ContextOrderMatches(*Actor, RouteSetup.ContextAction))
        bContextActionObserved = true;

    if (Command->type == sim::CommandType::Move &&
        (bDirect || bContext) &&
        Command->actor == RouteSetup.MoveActor &&
        IsAtDestination(Command->position, RouteSetup.MoveDestination,
            RouteSetup.MoveInputToleranceRaw) &&
        Actor->order.type == sim::OrderType::Move &&
        Actor->order.destination == Command->position)
    {
        if (!bMoveArrivalObserved)
            ObservedMoveDestination = Command->position;
        bMoveApplied = true;
        bMoveOrderObserved = true;
    }
    else if (Command->type == sim::CommandType::Stop &&
        bDirect &&
        Command->actor == RouteSetup.StopActor &&
        Receipt->assignedExecutionTick == PreviousStateTick &&
        StopOrderBeforeAdvance.type != sim::OrderType::None &&
        Actor->order.type == sim::OrderType::None)
        bStopObserved = true;
    else if (Command->type == sim::CommandType::Patrol &&
        bDirect &&
        Command->actor == RouteSetup.PatrolActor &&
        IsAtDestination(Command->position, RouteSetup.PatrolDestination,
            RouteSetup.PatrolInputToleranceRaw) &&
        Simulation.IsPositionPassable(Command->position) &&
        Actor->order.type == sim::OrderType::Patrol &&
        Actor->order.destination == Command->position)
        bPatrolObserved = true;
    else if (Command->type == sim::CommandType::Guard &&
        (bDirect || bContext) &&
        Command->actor == RouteSetup.GuardActor &&
        Command->target == RouteSetup.GuardTarget &&
        Actor->order.type == sim::OrderType::Guard &&
        Actor->order.target == RouteSetup.GuardTarget)
        bGuardObserved = true;
}

bool FEchoesTutorialOrderObservation::ObserveRejectedAttempt(
    uint64 Session,
    uint64 SnapshotToken,
    const sim::Simulation& Simulation,
    std::uint64_t InputAttemptSequence,
    const FEchoesTutorialExpectedCommand& Attempt)
{
    if (!AdvanceState(Session, SnapshotToken, Simulation) ||
        Mode != EMode::Route ||
        InputAttemptSequence < RouteSetup.FirstInputAttemptSequence ||
        ProcessedInputAttempts.Contains(InputAttemptSequence))
        return false;
    ProcessedInputAttempts.Add(InputAttemptSequence);
    if (Attempt.Type != RouteSetup.RejectedAction.Type ||
        Attempt.Actor != RouteSetup.RejectedAction.Actor ||
        Attempt.Target != RouteSetup.RejectedAction.Target ||
        !IsSameTerrainTile(
            Attempt.Position, RouteSetup.RejectedAction.Position) ||
        !IsAtDestination(Attempt.Position, RouteSetup.RejectedAction.Position,
            RouteSetup.RejectedInputToleranceRaw))
        return false;
    const sim::Entity* Actor = Simulation.FindEntity(Attempt.Actor);
    if (Actor == nullptr || Actor->owner != RouteSetup.LocalPlayer ||
        Actor->hitPoints <= 0 || !Actor->completed)
        return false;
    const sim::CommandResolutionOutcome Outcome =
        Simulation.ValidateMoveOrder(
            RouteSetup.LocalPlayer, Attempt.Actor, Attempt.Position);
    if (Outcome != RouteSetup.ExpectedRejectionOutcome ||
        Outcome == sim::CommandResolutionOutcome::Applied)
        return false;
    PendingRejectedAttemptSequence = InputAttemptSequence;
    return true;
}

void FEchoesTutorialOrderObservation::ObserveRejectionAcknowledged(
    uint64 Session,
    uint64 SnapshotToken,
    const sim::Simulation& Simulation,
    std::uint64_t InputAttemptSequence)
{
    if (!AdvanceState(Session, SnapshotToken, Simulation) ||
        Mode != EMode::Route || PendingRejectedAttemptSequence == 0 ||
        InputAttemptSequence != PendingRejectedAttemptSequence)
        return;
    bRejectionAcknowledged = true;
    PendingRejectedAttemptSequence = 0;
}

bool FEchoesTutorialOrderObservation::RouteSimulationPredicateSatisfied() const
{
    return IsActive() && Mode == EMode::Route &&
        RouteProgress().PredicateSatisfied();
}

bool FEchoesTutorialOrderObservation::ReserveSimulationPredicateSatisfied() const
{
    return IsActive() && Mode == EMode::Reserve &&
        ReserveProgress().PredicateSatisfied();
}

FEchoesTutorialRouteProgress
FEchoesTutorialOrderObservation::RouteProgress() const
{
    FEchoesTutorialRouteProgress Progress;
    if (!IsActive() || Mode != EMode::Route) return Progress;
    Progress.bMoveArrived =
        bMoveApplied && bMoveOrderObserved && bMoveArrivalObserved;
    Progress.bContextActionCompleted = bContextActionObserved;
    Progress.bStopCompleted = bStopObserved;
    Progress.bPatrolCompleted = bPatrolObserved;
    Progress.bGuardCompleted = bGuardObserved;
    Progress.bRejectionAcknowledged = bRejectionAcknowledged;
    return Progress;
}

FEchoesTutorialReserveProgress
FEchoesTutorialOrderObservation::ReserveProgress() const
{
    FEchoesTutorialReserveProgress Progress;
    if (!IsActive() || Mode != EMode::Reserve) return Progress;
    Progress.bGatherStarted = bGatherApplied && bGatherOrderObserved;
    Progress.bDeliverObserved = bDeliverOrderObserved;
    Progress.bContinuousRoute = bContinuousReserveRoute;
    Progress.DeliveredMatter = DeliveredMatter;
    return Progress;
}
