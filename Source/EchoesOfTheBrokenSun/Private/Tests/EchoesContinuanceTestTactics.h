#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"

#include "EchoesSimulationSubsystem.h"
#include "EchoesTermsOfContinuanceMissionModel.h"

namespace EchoesContinuanceTestTactics
{
inline bool AssignSpineDefense(
    UEchoesSimulationSubsystem* Bridge,
    echoes::sim::FutureWellChoice Choice,
    const FEchoesTermsOfContinuancePlan& Plan,
    FString& OutFeedback)
{
    using namespace echoes::sim;
    if (Choice != FutureWellChoice::Reshape)
    {
        return true;
    }
    if (Bridge == nullptr || Bridge->GetSimulation() == nullptr)
    {
        OutFeedback = TEXT("[M05_SPINE_DEFENSE] Missing simulation.");
        return false;
    }

    const FEchoesObjectiveSnapshot Objective =
        Bridge->GetLocalObjectiveSnapshot();
    int32 AssignedDefenders = 0;
    for (const Entity& Defender : Bridge->GetSimulation()->Entities())
    {
        if (Defender.owner != UEchoesSimulationSubsystem::LocalPlayerId ||
            (Defender.type != EntityType::Soldier &&
             Defender.type != EntityType::HeavyUnit &&
             Defender.type != EntityType::ScoutUnit) ||
            Defender.id == Objective.MeridianContinuanceWitnessId ||
            Defender.id == Objective.KharuunContinuanceWitnessId)
        {
            continue;
        }
        FString CommandFeedback;
        if (!Bridge->IssueCommand(
                CommandType::Guard,
                Defender.id,
                Objective.KharuunContinuanceSpineId,
                Bridge->SimToWorld(Plan.KharuunSpineSite),
                FutureWellChoice::Dormant,
                CommandFeedback))
        {
            OutFeedback = CommandFeedback;
            return false;
        }
        ++AssignedDefenders;
    }
    if (AssignedDefenders != 5)
    {
        OutFeedback = FString::Printf(
            TEXT("[M05_SPINE_DEFENSE] Expected five defenders; assigned %d."),
            AssignedDefenders);
        return false;
    }
    return true;
}

inline bool MaintainSpineDefense(
    UEchoesSimulationSubsystem* Bridge,
    echoes::sim::FutureWellChoice Choice,
    const FEchoesTermsOfContinuancePlan& Plan,
    FString& OutFeedback)
{
    using namespace echoes::sim;
    if (Choice != FutureWellChoice::Reshape)
    {
        return true;
    }
    const Simulation* Current = Bridge != nullptr
        ? Bridge->GetSimulation()
        : nullptr;
    if (Current == nullptr)
    {
        OutFeedback = TEXT("[M05_SPINE_DEFENSE] Missing simulation.");
        return false;
    }
    if (Current->CurrentTick() % 20 != 0)
    {
        return true;
    }

    const auto View = Current->CreatePlayerView(
        UEchoesSimulationSubsystem::LocalPlayerId);
    if (!View.has_value())
    {
        OutFeedback = TEXT("[M05_SPINE_DEFENSE] Missing local player view.");
        return false;
    }
    const Entity* Contact = nullptr;
    int64 BestDistance = TNumericLimits<int64>::Max();
    const int64 RadiusRaw = 14LL * kFixedScale;
    for (const Entity& Visible : View->Entities())
    {
        if (!Current->Config().IsHostile(
                UEchoesSimulationSubsystem::LocalPlayerId,
                Visible.owner))
        {
            continue;
        }
        const int64 DeltaX =
            static_cast<int64>(Visible.position.x.Raw()) -
            Plan.KharuunSpineSite.x.Raw();
        const int64 DeltaY =
            static_cast<int64>(Visible.position.y.Raw()) -
            Plan.KharuunSpineSite.y.Raw();
        const int64 Distance = DeltaX * DeltaX + DeltaY * DeltaY;
        if (Distance < BestDistance && Distance <= RadiusRaw * RadiusRaw)
        {
            Contact = &Visible;
            BestDistance = Distance;
        }
    }

    const FEchoesObjectiveSnapshot Objective =
        Bridge->GetLocalObjectiveSnapshot();
    for (const Entity& Defender : Current->Entities())
    {
        if (Defender.owner != UEchoesSimulationSubsystem::LocalPlayerId ||
            (Defender.type != EntityType::Soldier &&
             Defender.type != EntityType::HeavyUnit &&
             Defender.type != EntityType::ScoutUnit) ||
            Defender.id == Objective.MeridianContinuanceWitnessId ||
            Defender.id == Objective.KharuunContinuanceWitnessId)
        {
            continue;
        }
        const EntityId Target = Contact != nullptr
            ? Contact->id
            : Objective.KharuunContinuanceSpineId;
        const CommandType Mode = Contact != nullptr
            ? CommandType::AttackMove
            : CommandType::Guard;
        const OrderType ExpectedOrder = Contact != nullptr
            ? OrderType::AttackMove
            : OrderType::Guard;
        if (Defender.order.type == ExpectedOrder &&
            (Contact != nullptr
                ? Defender.order.destination == Contact->position
                : Defender.order.target == Target))
        {
            continue;
        }
        FString CommandFeedback;
        if (!Bridge->IssueCommand(
                Mode,
                Defender.id,
                Target,
                Bridge->SimToWorld(
                    Contact != nullptr
                        ? Contact->position
                        : Plan.KharuunSpineSite),
                FutureWellChoice::Dormant,
                CommandFeedback))
        {
            OutFeedback = CommandFeedback;
            return false;
        }
    }
    return true;
}
} // namespace EchoesContinuanceTestTactics

#endif
