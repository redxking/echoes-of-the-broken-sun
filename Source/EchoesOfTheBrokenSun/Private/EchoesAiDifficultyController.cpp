#include "EchoesAiDifficultyController.h"

#include <algorithm>
#include <limits>
#include <unordered_set>

namespace
{
using echoes::sim::Command;
using echoes::sim::PlayerId;
using echoes::sim::Tick;

[[nodiscard]] Tick SaturatingAdd(Tick Left, Tick Right)
{
    const Tick Maximum = std::numeric_limits<Tick>::max();
    return Left > Maximum - Right ? Maximum : Left + Right;
}

[[nodiscard]] Tick LatestPendingExecutionTick(
    PlayerId Player,
    std::span<const Command> PendingCommands)
{
    Tick Latest = 0;
    for (const Command& Pending : PendingCommands)
    {
        if (Pending.player == Player)
        {
            Latest = std::max(Latest, Pending.executeTick);
        }
    }
    return Latest;
}
}

bool FEchoesAiDifficultyController::IsGroupCommandTick(
    echoes::sim::Tick Tick,
    uint32 TicksPerSecond,
    int32 GroupCommandsPerSecond)
{
    if (TicksPerSecond == 0 || GroupCommandsPerSecond <= 0)
    {
        return false;
    }
    const uint64 Ceiling = FMath::Min<uint64>(
        static_cast<uint64>(GroupCommandsPerSecond),
        static_cast<uint64>(TicksPerSecond));
    if (Tick == 0)
    {
        return true;
    }

    // This discrete balanced schedule has exactly Ceiling admissions in each
    // aligned second and no more than Ceiling in any rolling second.
    const uint64 CurrentSlot =
        (Tick % TicksPerSecond) * Ceiling / TicksPerSecond;
    const uint64 PreviousTickInSecond = (Tick - 1) % TicksPerSecond;
    const uint64 PreviousSlot =
        PreviousTickInSecond * Ceiling / TicksPerSecond;
    return Tick % TicksPerSecond == 0 || CurrentSlot != PreviousSlot;
}

bool FEchoesAiDifficultyController::IsStrategicReviewTick(
    echoes::sim::Tick Tick,
    const FEchoesAiDifficultyPolicy& Policy)
{
    return Policy.StrategicReviewTicks > 0 &&
        Tick % Policy.StrategicReviewTicks == 0;
}

bool FEchoesAiDifficultyController::IsStrategicCommand(
    echoes::sim::CommandType Type)
{
    switch (Type)
    {
        case echoes::sim::CommandType::Build:
        case echoes::sim::CommandType::Produce:
        case echoes::sim::CommandType::Research:
        case echoes::sim::CommandType::FutureWell:
        case echoes::sim::CommandType::AdaptWarform:
        case echoes::sim::CommandType::ReconcileToManifest:
        case echoes::sim::CommandType::ReconcileToPossible:
            return true;
        default:
            return false;
    }
}

FEchoesAiDifficultyPlan FEchoesAiDifficultyController::BuildPlan(
    const echoes::sim::PlayerView& View,
    echoes::sim::AiPersonality Personality,
    const FEchoesAiDifficultyPolicy& Policy,
    std::span<const echoes::sim::Command> PendingCommands)
{
    FEchoesAiDifficultyPlan Plan;
    Plan.ObservationTick = View.CurrentTick();
    Plan.GroupCommandsPerSecond = FMath::Max(0, Policy.GroupCommandsPerSecond);
    Plan.bGroupCommandWindowOpen = IsGroupCommandTick(
        Plan.ObservationTick,
        View.Config().ticksPerSecond,
        Plan.GroupCommandsPerSecond);
    Plan.bStrategicReview =
        IsStrategicReviewTick(Plan.ObservationTick, Policy);
    if (!Plan.bGroupCommandWindowOpen)
    {
        return Plan;
    }

    const PlayerId Player = View.Player().id;
    std::unordered_set<echoes::sim::EntityId> PendingActors;
    PendingActors.reserve(PendingCommands.size());
    for (const Command& Pending : PendingCommands)
    {
        if (Pending.player == Player)
        {
            PendingActors.insert(Pending.actor);
        }
    }

    const std::vector<Command> Generated =
        echoes::sim::Simulation::GenerateAiCommands(View, Personality);
    std::vector<Command> Strategic;
    std::vector<Command> Tactical;
    Strategic.reserve(Generated.size());
    Tactical.reserve(Generated.size());
    for (const Command& Candidate : Generated)
    {
        if (Candidate.player != Player ||
            PendingActors.contains(Candidate.actor))
        {
            continue;
        }
        if (IsStrategicCommand(Candidate.type))
        {
            if (Plan.bStrategicReview)
            {
                Strategic.push_back(Candidate);
            }
        }
        else
        {
            Tactical.push_back(Candidate);
        }
    }

    std::vector<Command>& Eligible =
        !Strategic.empty() ? Strategic : Tactical;
    if (Eligible.empty())
    {
        return Plan;
    }

    // Rotating selection prevents an always-busy low entity ID from owning
    // every admission when more actors are ready than the selected rate cap.
    const uint64 TicksPerSecond =
        FMath::Max<uint32>(1U, View.Config().ticksPerSecond);
    const uint64 CommandRate =
        static_cast<uint64>(Plan.GroupCommandsPerSecond);
    const uint64 EligibleCount = static_cast<uint64>(Eligible.size());
    const uint64 WholeSecondRotation =
        ((Plan.ObservationTick / TicksPerSecond) % EligibleCount) *
        (CommandRate % EligibleCount) % EligibleCount;
    const uint64 WithinSecondRotation =
        ((Plan.ObservationTick % TicksPerSecond) * CommandRate /
         TicksPerSecond) % EligibleCount;
    Command Selected = Eligible[static_cast<size_t>(
        (WholeSecondRotation + WithinSecondRotation) % EligibleCount)];
    const Tick ReactionExecutionTick = SaturatingAdd(
        Plan.ObservationTick,
        Policy.ReactionTicks);
    // A difficulty change may leave slower-policy work scheduled farther out.
    // Keeping the new input at that execution frontier preserves the core's
    // sequence-order invariant. Stable-policy decisions use the exact delay.
    Plan.ScheduledExecutionTick = std::max(
        ReactionExecutionTick,
        LatestPendingExecutionTick(Player, PendingCommands));
    Selected.executeTick = Plan.ScheduledExecutionTick;
    Plan.Commands.push_back(Selected);
    return Plan;
}
