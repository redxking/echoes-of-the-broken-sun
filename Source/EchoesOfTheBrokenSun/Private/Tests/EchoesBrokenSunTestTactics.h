#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"

#include "EchoesBrokenSunMissionModel.h"
#include "EchoesSimulationSubsystem.h"

namespace EchoesBrokenSunTestTactics
{
inline FString DescribeBrokenSunEntity(
    const TCHAR* Label,
    echoes::sim::EntityId Id,
    const echoes::sim::Entity* Current)
{
    if (Current == nullptr)
    {
        return FString::Printf(TEXT("%s{id=%u missing}"), Label, Id);
    }
    return FString::Printf(
        TEXT("%s{id=%u owner=%u faction=%u type=%u hp=%d/%d pos=(%d,%d) completed=%s progress=%d/%d order=%u target=%u anchor=(%d,%d) destination=(%d,%d)}"),
        Label,
        Id,
        Current->owner,
        static_cast<uint8>(Current->faction),
        static_cast<uint8>(Current->type),
        Current->hitPoints,
        Current->maxHitPoints,
        Current->position.x.FloorToInt(),
        Current->position.y.FloorToInt(),
        Current->completed ? TEXT("true") : TEXT("false"),
        Current->constructionProgress,
        Current->constructionRequired,
        static_cast<uint8>(Current->order.type),
        Current->order.target,
        Current->order.anchor.x.FloorToInt(),
        Current->order.anchor.y.FloorToInt(),
        Current->order.destination.x.FloorToInt(),
        Current->order.destination.y.FloorToInt());
}

inline TArray<echoes::sim::EntityId> FindBrokenSunNemeGuards(
    const UEchoesSimulationSubsystem* Bridge,
    echoes::sim::EntityId AccordVoiceId)
{
    TArray<echoes::sim::EntityId> Guards;
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (Simulation == nullptr)
    {
        return Guards;
    }
    for (const echoes::sim::Entity& Candidate : Simulation->Entities())
    {
        if (Candidate.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Candidate.faction == echoes::sim::Faction::HollowChoir &&
            Candidate.type == echoes::sim::EntityType::Soldier &&
            Candidate.id != AccordVoiceId && Candidate.hitPoints > 0)
        {
            Guards.Add(Candidate.id);
        }
    }
    Guards.Sort();
    if (Guards.Num() > 2)
    {
        Guards.SetNum(2, EAllowShrinking::No);
    }
    return Guards;
}

inline bool BrokenSunEntityWithinTiles(
    const UEchoesSimulationSubsystem* Bridge,
    echoes::sim::EntityId EntityId,
    const echoes::sim::Vec2& Site,
    int32 RadiusTiles)
{
    const echoes::sim::Entity* Current = Bridge->FindEntity(EntityId);
    if (Current == nullptr || Current->hitPoints <= 0)
    {
        return false;
    }
    const int64 DeltaX = static_cast<int64>(Current->position.x.Raw()) -
        Site.x.Raw();
    const int64 DeltaY = static_cast<int64>(Current->position.y.Raw()) -
        Site.y.Raw();
    const int64 RadiusRaw =
        static_cast<int64>(RadiusTiles) * echoes::sim::kFixedScale;
    return DeltaX * DeltaX + DeltaY * DeltaY <= RadiusRaw * RadiusRaw;
}

inline FString BrokenSunTacticalDiagnostic(
    const UEchoesSimulationSubsystem* Bridge,
    const TCHAR* Context,
    echoes::sim::EntityId NemeId,
    const TArray<echoes::sim::EntityId>& Guards)
{
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    const FEchoesObjectiveSnapshot Objective =
        Bridge->GetLocalObjectiveSnapshot();
    return FString::Printf(
        TEXT("[M15_TACTICAL_FAILURE] context=%s reason=%s phase=%s tick=%llu %s %s %s %s %s %s %s %s"),
        Context,
        *Bridge->GetMissionFailureReasonCode(),
        FEchoesBrokenSunMissionModel::StableName(
            Bridge->GetBrokenSunPhase()),
        static_cast<unsigned long long>(
            Simulation != nullptr ? Simulation->CurrentTick() : 0),
        *DescribeBrokenSunEntity(
            TEXT("mara"),
            Objective.BrokenSunMaraId,
            Bridge->FindEntity(Objective.BrokenSunMaraId)),
        *DescribeBrokenSunEntity(
            TEXT("oruun"),
            Objective.BrokenSunOruunId,
            Bridge->FindEntity(Objective.BrokenSunOruunId)),
        *DescribeBrokenSunEntity(
            TEXT("talar"),
            Objective.BrokenSunTalarId,
            Bridge->FindEntity(Objective.BrokenSunTalarId)),
        *DescribeBrokenSunEntity(
            TEXT("voice"),
            Objective.BrokenSunAccordVoiceId,
            Bridge->FindEntity(Objective.BrokenSunAccordVoiceId)),
        *DescribeBrokenSunEntity(
            TEXT("heavy"),
            Objective.BrokenSunAccordHeavyId,
            Bridge->FindEntity(Objective.BrokenSunAccordHeavyId)),
        *DescribeBrokenSunEntity(
            TEXT("neme"), NemeId, Bridge->FindEntity(NemeId)),
        *DescribeBrokenSunEntity(
            TEXT("nemeGuard1"),
            Guards.IsValidIndex(0) ? Guards[0] : 0,
            Guards.IsValidIndex(0) ? Bridge->FindEntity(Guards[0]) : nullptr),
        *DescribeBrokenSunEntity(
            TEXT("nemeGuard2"),
            Guards.IsValidIndex(1) ? Guards[1] : 0,
            Guards.IsValidIndex(1) ? Bridge->FindEntity(Guards[1]) : nullptr));
}

inline FString BrokenSunResearchAdmissionDiagnostic(
    UEchoesSimulationSubsystem* Bridge,
    echoes::sim::EntityId ProducerId,
    echoes::sim::ResearchType Research,
    const FString& CommandFeedback)
{
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(
                  UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    const echoes::sim::Entity* Producer =
        Bridge != nullptr ? Bridge->FindEntity(ProducerId) : nullptr;
    const int32 Admission =
        Simulation != nullptr
            ? static_cast<int32>(Simulation->ValidateResearch(
                  UEchoesSimulationSubsystem::LocalPlayerId,
                  ProducerId,
                  Research))
            : -1;
    return FString::Printf(
        TEXT("[M15_RESEARCH_ADMISSION] tick=%llu phase=%u research=%u result=%d completedMask=0x%08X activeResearch=%d progress=%d/%d researchProducer=%u resources=%d/%d loomProduction=%d/%d loomQueue=%d loom=%s commandFeedback=%s"),
        static_cast<unsigned long long>(
            Simulation != nullptr ? Simulation->CurrentTick() : 0),
        static_cast<uint8>(
            Bridge != nullptr ? Bridge->GetBrokenSunPhase()
                              : EEchoesBrokenSunPhase::Failed),
        static_cast<uint8>(Research),
        Admission,
        Player != nullptr ? Player->completedResearchMask : 0U,
        Player != nullptr
            ? static_cast<int32>(Player->activeResearch)
            : -1,
        Player != nullptr ? Player->researchProgress : -1,
        Player != nullptr ? Player->researchRequired : -1,
        Player != nullptr ? Player->researchProducer : 0,
        Player != nullptr ? Player->resources.material : -1,
        Player != nullptr ? Player->resources.dawnshards : -1,
        Producer != nullptr ? Producer->productionProgress : -1,
        Producer != nullptr ? Producer->productionRequired : -1,
        Producer != nullptr
            ? static_cast<int32>(Producer->orderQueue.size())
            : -1,
        *DescribeBrokenSunEntity(TEXT("loom"), ProducerId, Producer),
        *CommandFeedback);
}

inline bool MaintainBrokenSunNemeGuards(
    UEchoesSimulationSubsystem* Bridge,
    echoes::sim::EntityId NemeId,
    const TArray<echoes::sim::EntityId>& Guards,
    FString& OutFeedback)
{
    const echoes::sim::Entity* Neme = Bridge->FindEntity(NemeId);
    if (Guards.Num() != 2 || Neme == nullptr || Neme->hitPoints <= 0)
    {
        OutFeedback = BrokenSunTacticalDiagnostic(
            Bridge, TEXT("guarded-unit-loss"), NemeId, Guards);
        return false;
    }
    for (const echoes::sim::EntityId GuardId : Guards)
    {
        const echoes::sim::Entity* Guard = Bridge->FindEntity(GuardId);
        if (Guard == nullptr || Guard->hitPoints <= 0)
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge, TEXT("escort-loss"), NemeId, Guards);
            return false;
        }
        if (Guard->order.type != echoes::sim::OrderType::Guard ||
            Guard->order.target != NemeId)
        {
            FString CommandFeedback;
            if (!Bridge->IssueCommand(
                    echoes::sim::CommandType::Guard,
                    GuardId,
                    NemeId,
                    Bridge->SimToWorld(Neme->position),
                    echoes::sim::FutureWellChoice::Dormant,
                    CommandFeedback))
            {
                OutFeedback = BrokenSunTacticalDiagnostic(
                    Bridge, TEXT("guard-reassertion"), NemeId, Guards);
                OutFeedback += FString::Printf(
                    TEXT(" commandFeedback=%s"), *CommandFeedback);
                return false;
            }
        }
    }
    return true;
}

inline bool PaceBrokenSunNemeToSite(
    UEchoesSimulationSubsystem* Bridge,
    echoes::sim::EntityId NemeId,
    const TArray<echoes::sim::EntityId>& Guards,
    const echoes::sim::Vec2& PublicGoal,
    int32 MaximumTicks,
    FString& OutFeedback)
{
    constexpr int32 StepRaw = 2 * echoes::sim::kFixedScale;
    const echoes::sim::Vec2 NemeGoal = echoes::sim::Vec2::FromRaw(
        PublicGoal.x.Raw() - 2 * echoes::sim::kFixedScale,
        PublicGoal.y.Raw() - 2 * echoes::sim::kFixedScale);
    OutFeedback.Reset();
    int32 RemainingTicks = MaximumTicks;
    const auto GuardsAtGoal = [Bridge, &Guards, &PublicGoal]()
    {
        if (Guards.Num() != 2)
        {
            return false;
        }
        for (const echoes::sim::EntityId GuardId : Guards)
        {
            if (!BrokenSunEntityWithinTiles(
                    Bridge, GuardId, PublicGoal, 3))
            {
                return false;
            }
        }
        return true;
    };
    if (GuardsAtGoal())
    {
        const echoes::sim::Entity* Neme = Bridge->FindEntity(NemeId);
        FString CommandFeedback;
        if (Neme == nullptr ||
            !Bridge->IssueCommand(
                echoes::sim::CommandType::Move,
                NemeId,
                0,
                Bridge->SimToWorld(NemeGoal),
                echoes::sim::FutureWellChoice::Dormant,
                CommandFeedback))
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge, TEXT("screened-convoy-command"), NemeId, Guards);
            OutFeedback += FString::Printf(
                TEXT(" commandFeedback=%s"), *CommandFeedback);
            return false;
        }
        // The defended stand is sqrt(8) tiles from the public site; exact
        // arrival keeps the authoritative three-tile contract.
        while (RemainingTicks > 0 &&
               !BrokenSunEntityWithinTiles(
                   Bridge, NemeId, NemeGoal, 0))
        {
            if (Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::Failed ||
                Bridge->FindEntity(NemeId) == nullptr ||
                Bridge->FindEntity(NemeId)->hitPoints <= 0 ||
                !GuardsAtGoal())
            {
                OutFeedback = BrokenSunTacticalDiagnostic(
                    Bridge, TEXT("screened-convoy-loss"), NemeId, Guards);
                return false;
            }
            Bridge->Tick(0.05f);
            --RemainingTicks;
        }
        if (!BrokenSunEntityWithinTiles(Bridge, NemeId, NemeGoal, 0) ||
            !BrokenSunEntityWithinTiles(
                Bridge, NemeId, PublicGoal, 3))
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge, TEXT("screened-convoy-budget"), NemeId, Guards);
            OutFeedback += FString::Printf(
                TEXT(" publicGoal=(%d,%d) defendedNemeGoal=(%d,%d)"),
                PublicGoal.x.FloorToInt(),
                PublicGoal.y.FloorToInt(),
                NemeGoal.x.FloorToInt(),
                NemeGoal.y.FloorToInt());
            return false;
        }
        return true;
    }
    const auto EscortsReformed = [Bridge, NemeId, &Guards]()
    {
        const echoes::sim::Entity* Neme = Bridge->FindEntity(NemeId);
        if (Neme == nullptr || Neme->hitPoints <= 0 || Guards.Num() != 2)
        {
            return false;
        }
        for (const echoes::sim::EntityId GuardId : Guards)
        {
            const echoes::sim::Entity* Guard = Bridge->FindEntity(GuardId);
            if (Guard == nullptr || Guard->hitPoints <= 0 ||
                Guard->order.type != echoes::sim::OrderType::Guard ||
                Guard->order.target != NemeId ||
                !BrokenSunEntityWithinTiles(
                    Bridge, GuardId, Neme->position, 3))
            {
                return false;
            }
        }
        return true;
    };
    while (RemainingTicks > 0)
    {
        while (RemainingTicks > 0 && !EscortsReformed())
        {
            if (Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::Failed ||
                !MaintainBrokenSunNemeGuards(
                    Bridge, NemeId, Guards, OutFeedback))
            {
                if (OutFeedback.IsEmpty())
                {
                    OutFeedback = BrokenSunTacticalDiagnostic(
                        Bridge, TEXT("convoy-regroup"), NemeId, Guards);
                }
                return false;
            }
            Bridge->Tick(0.05f);
            --RemainingTicks;
        }
        if (BrokenSunEntityWithinTiles(Bridge, NemeId, NemeGoal, 0) &&
            BrokenSunEntityWithinTiles(Bridge, NemeId, PublicGoal, 3))
        {
            return true;
        }
        const echoes::sim::Entity* Neme = Bridge->FindEntity(NemeId);
        if (Neme == nullptr || Neme->hitPoints <= 0 || RemainingTicks <= 0)
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge, TEXT("convoy-witness"), NemeId, Guards);
            return false;
        }
        const echoes::sim::Vec2 StepStart = Neme->position;
        FString CommandFeedback;
        if (!Bridge->IssueCommand(
                echoes::sim::CommandType::Move,
                NemeId,
                0,
                Bridge->SimToWorld(NemeGoal),
                echoes::sim::FutureWellChoice::Dormant,
                CommandFeedback))
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge, TEXT("convoy-move"), NemeId, Guards);
            OutFeedback += FString::Printf(
                TEXT(" commandFeedback=%s"), *CommandFeedback);
            return false;
        }
        bool bStepComplete = false;
        while (RemainingTicks > 0 && !bStepComplete)
        {
            if (Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::Failed ||
                !MaintainBrokenSunNemeGuards(
                    Bridge, NemeId, Guards, OutFeedback))
            {
                if (OutFeedback.IsEmpty())
                {
                    OutFeedback = BrokenSunTacticalDiagnostic(
                        Bridge, TEXT("convoy-step"), NemeId, Guards);
                }
                return false;
            }
            Bridge->Tick(0.05f);
            --RemainingTicks;
            Neme = Bridge->FindEntity(NemeId);
            if (Neme == nullptr || Neme->hitPoints <= 0)
            {
                OutFeedback = BrokenSunTacticalDiagnostic(
                    Bridge, TEXT("convoy-witness"), NemeId, Guards);
                return false;
            }
            const int64 DeltaX =
                static_cast<int64>(Neme->position.x.Raw()) -
                StepStart.x.Raw();
            const int64 DeltaY =
                static_cast<int64>(Neme->position.y.Raw()) -
                StepStart.y.Raw();
            bStepComplete = BrokenSunEntityWithinTiles(
                Bridge, NemeId, NemeGoal, 0) ||
                DeltaX * DeltaX + DeltaY * DeltaY >=
                    static_cast<int64>(StepRaw) * StepRaw;
        }
        if (!BrokenSunEntityWithinTiles(Bridge, NemeId, NemeGoal, 0))
        {
            Neme = Bridge->FindEntity(NemeId);
            if (Neme == nullptr ||
                !Bridge->IssueCommand(
                    echoes::sim::CommandType::Stop,
                    NemeId,
                    0,
                    Bridge->SimToWorld(Neme->position),
                    echoes::sim::FutureWellChoice::Dormant,
                    CommandFeedback))
            {
                OutFeedback = BrokenSunTacticalDiagnostic(
                    Bridge, TEXT("convoy-stop"), NemeId, Guards);
                return false;
            }
        }
    }
    OutFeedback = BrokenSunTacticalDiagnostic(
        Bridge, TEXT("convoy-budget"), NemeId, Guards);
    OutFeedback += FString::Printf(
        TEXT(" publicGoal=(%d,%d) defendedNemeGoal=(%d,%d)"),
        PublicGoal.x.FloorToInt(),
        PublicGoal.y.FloorToInt(),
        NemeGoal.x.FloorToInt(),
        NemeGoal.y.FloorToInt());
    return false;
}

inline bool ClearBrokenSunNemeAccordApproach(
    UEchoesSimulationSubsystem* Bridge,
    const FEchoesObjectiveSnapshot& Objective,
    const TArray<echoes::sim::EntityId>& Guards,
    const echoes::sim::Vec2& PublicAccordSite,
    int32 MaximumTicks,
    FString& OutFeedback)
{
    using echoes::sim::CommandType;
    using echoes::sim::EntityId;
    using echoes::sim::FutureWellChoice;

    OutFeedback.Reset();
    if (Bridge == nullptr || Bridge->GetSimulation() == nullptr ||
        Guards.Num() != 2)
    {
        OutFeedback = TEXT(
            "[M15_CLEARANCE_FAILED] reason=missing-simulation-or-escorts");
        return false;
    }

    const TArray<EntityId> CombatActors = {
        Objective.BrokenSunAccordVoiceId,
        Guards[0],
        Guards[1],
        Objective.BrokenSunAccordHeavyId};
    const TArray<EntityId> ProtectedActors = {
        Objective.BrokenSunMaraId,
        Objective.BrokenSunOruunId,
        Objective.BrokenSunTalarId,
        Objective.BrokenSunAccordVoiceId,
        Objective.BrokenSunAccordHeavyId,
        Objective.BrokenSunNemeId};

    const auto ActorsIntact = [Bridge](const TArray<EntityId>& Actors)
    {
        for (const EntityId ActorId : Actors)
        {
            const echoes::sim::Entity* Actor = Bridge->FindEntity(ActorId);
            if (Actor == nullptr || Actor->hitPoints <= 0)
            {
                return false;
            }
        }
        return true;
    };
    const auto IssueSiteOrder = [&](EntityId ActorId, CommandType Type)
    {
        const echoes::sim::Entity* Actor = Bridge->FindEntity(ActorId);
        FString CommandFeedback;
        if (Actor == nullptr ||
            !Bridge->IssueCommand(
                Type,
                ActorId,
                0,
                Bridge->SimToWorld(
                    Type == CommandType::Hold
                        ? Actor->position
                        : PublicAccordSite),
                FutureWellChoice::Dormant,
                CommandFeedback))
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge,
                TEXT("clearance-command"),
                Objective.BrokenSunNemeId,
                Guards);
            OutFeedback += FString::Printf(
                TEXT(" actor=%u command=%u commandFeedback=%s"),
                ActorId,
                static_cast<uint8>(Type),
                *CommandFeedback);
            return false;
        }
        return true;
    };
    bool bInitialRallyAccepted = true;
    if (ActorsIntact(CombatActors) && ActorsIntact(ProtectedActors))
    {
        for (const EntityId ActorId : CombatActors)
        {
            bInitialRallyAccepted =
                IssueSiteOrder(ActorId, CommandType::Move) &&
                bInitialRallyAccepted;
        }
    }
    if (!ActorsIntact(CombatActors) || !ActorsIntact(ProtectedActors) ||
        !bInitialRallyAccepted)
    {
        if (OutFeedback.IsEmpty())
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge,
                TEXT("clearance-initial-loss"),
                Objective.BrokenSunNemeId,
                Guards);
        }
        return false;
    }

    constexpr int32 DefenseRadiusTiles = 6;
    constexpr int32 RequiredQuietTicks = 40;
    int32 LastVisibleThreatCount = 0;
    int32 QuietTicks = 0;
    for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
    {
        if (Bridge->GetBrokenSunPhase() ==
                EEchoesBrokenSunPhase::Failed ||
            !ActorsIntact(CombatActors) || !ActorsIntact(ProtectedActors))
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge,
                TEXT("clearance-protected-loss"),
                Objective.BrokenSunNemeId,
                Guards);
            return false;
        }

        const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
        const std::optional<echoes::sim::PlayerView> LocalView =
            Simulation != nullptr
                ? Simulation->CreatePlayerView(
                      UEchoesSimulationSubsystem::LocalPlayerId)
                : std::nullopt;
        if (!LocalView.has_value())
        {
            OutFeedback = BrokenSunTacticalDiagnostic(
                Bridge,
                TEXT("clearance-view"),
                Objective.BrokenSunNemeId,
                Guards);
            return false;
        }

        const int64 ThreatRadiusRaw =
            static_cast<int64>(DefenseRadiusTiles) *
            echoes::sim::kFixedScale;
        LastVisibleThreatCount = 0;
        for (const echoes::sim::Entity& Visible : LocalView->Entities())
        {
            if (!LocalView->Config().IsHostile(
                    UEchoesSimulationSubsystem::LocalPlayerId,
                    Visible.owner) ||
                Visible.hitPoints <= 0)
            {
                continue;
            }
            const int64 DeltaX =
                static_cast<int64>(Visible.position.x.Raw()) -
                PublicAccordSite.x.Raw();
            const int64 DeltaY =
                static_cast<int64>(Visible.position.y.Raw()) -
                PublicAccordSite.y.Raw();
            if (DeltaX * DeltaX + DeltaY * DeltaY <=
                ThreatRadiusRaw * ThreatRadiusRaw)
            {
                ++LastVisibleThreatCount;
            }
        }

        bool bForceAtPublicSite = true;
        for (const EntityId ActorId : CombatActors)
        {
            const echoes::sim::Entity* Actor = Bridge->FindEntity(ActorId);
            const bool bActorAtPublicSite =
                BrokenSunEntityWithinTiles(
                    Bridge, ActorId, PublicAccordSite, 3);
            bForceAtPublicSite = bActorAtPublicSite && bForceAtPublicSite;
            const echoes::sim::OrderType DesiredOrder =
                bActorAtPublicSite
                    ? echoes::sim::OrderType::Hold
                    : echoes::sim::OrderType::Move;
            if (Actor == nullptr || Actor->order.type != DesiredOrder)
            {
                if (!IssueSiteOrder(
                        ActorId,
                        bActorAtPublicSite
                            ? CommandType::Hold
                            : CommandType::Move))
                {
                    return false;
                }
            }
        }
        if (bForceAtPublicSite && LastVisibleThreatCount == 0)
        {
            ++QuietTicks;
            if (QuietTicks >= RequiredQuietTicks)
            {
                return true;
            }
        }
        else
        {
            QuietTicks = 0;
        }
        Bridge->Tick(0.05f);
    }

    OutFeedback = BrokenSunTacticalDiagnostic(
        Bridge,
        TEXT("clearance-budget"),
        Objective.BrokenSunNemeId,
        Guards);
    OutFeedback += FString::Printf(
        TEXT(" visibleLocalThreats=%d quietTicks=%d requiredQuietTicks=%d defenseRadius=%d budget=%d publicSite=(%d,%d)"),
        LastVisibleThreatCount,
        QuietTicks,
        RequiredQuietTicks,
        DefenseRadiusTiles,
        MaximumTicks,
        PublicAccordSite.x.FloorToInt(),
        PublicAccordSite.y.FloorToInt());
    return false;
}

} // namespace EchoesBrokenSunTestTactics

#endif
