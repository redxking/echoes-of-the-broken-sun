#include "EchoesPlayerController.h"
#include "EchoesCommandMarkerView.h"

#include "EchoesEntityView.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

namespace
{
bool IsMaintenanceStructure(echoes::sim::EntityType Type)
{
    using echoes::sim::EntityType;
    return Type == EntityType::CommandCore || Type == EntityType::Dropoff ||
        Type == EntityType::Barracks || Type == EntityType::UtilityStructure;
}
}

bool AEchoesPlayerController::TryIssueWorkerMaintenanceContext(uint32 TargetId)
{
    using namespace echoes::sim;
    PruneSelection();
    bool bHasWorker = false;
    bool bAssist = false;
    if (GetNetMode() == NM_Client)
    {
        const auto* Target = FindNetworkEntity(TargetId);
        if (!Target || Target->owner != NetworkSeat || Target->hitPoints <= 0)
            return false;
        bAssist = !Target->completed && IsMaintenanceStructure(Target->type);
        if (!bAssist && Target->hitPoints >= Target->maxHitPoints) return false;
        for (const uint32 Id : SelectedEntityIds)
        {
            const auto* Actor = FindNetworkEntity(Id);
            bHasWorker |= Actor && Actor->owner == NetworkSeat && Actor->type == EntityType::Worker;
        }
    }
    else
    {
        const auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
        const auto* Sim = Bridge ? Bridge->GetSimulation() : nullptr;
        const auto* Target = Sim ? Sim->FindEntity(TargetId) : nullptr;
        if (!Target || Target->owner == kNeutralPlayer || Target->hitPoints <= 0 ||
            Sim->Config().IsHostile(UEchoesSimulationSubsystem::LocalPlayerId, Target->owner) ||
            !Sim->IsEntityVisibleTo(UEchoesSimulationSubsystem::LocalPlayerId, TargetId)) return false;
        bAssist = Target->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            !Target->completed && IsMaintenanceStructure(Target->type);
        if (!bAssist && Target->hitPoints >= Target->maxHitPoints) return false;
        for (const uint32 Id : SelectedEntityIds)
        {
            const auto* Actor = Sim->FindEntity(Id);
            bHasWorker |= Actor && Actor->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Actor->type == EntityType::Worker;
        }
    }
    if (!bHasWorker) return false;
    (void)IssueSelectedWorkerMaintenance(TargetId, bAssist);
    return true;
}

bool AEchoesPlayerController::IssueSelectedWorkerMaintenance(uint32 TargetId, bool bConstructionAssist)
{
    using namespace echoes::sim;
    if (IsReplayInputActive() || IsModalOverlayVisible()) return false;
    if (bTutorialOperationAuthorized && (GetTutorialProgressMask() & 2) == 0)
    {
        SetStatusMessage(TEXT("Follow the active tutorial step before issuing orders."));
        return false;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        const auto* Target = FindNetworkEntity(TargetId);
        if (!IsNetworkClientControlActive() || !Target)
        {
            SetStatusMessage(TEXT("Choose a visible maintenance target after the battlefield reconnects."));
            return false;
        }
        TArray<net::CommandIntent> Intents;
        for (const uint32 Id : SelectedEntityIds)
        {
            const auto* Actor = FindNetworkEntity(Id);
            if (!Actor || Actor->owner != NetworkSeat || Actor->type != EntityType::Worker) continue;
            net::CommandIntent Intent;
            Intent.type = bConstructionAssist ? CommandType::Build : CommandType::Repair;
            Intent.actor = Id;
            Intent.target = TargetId;
            Intent.position = Target->position;
            Intent.buildType = Target->type;
            Intents.Add(Intent);
        }
        if (Intents.IsEmpty())
        {
            SetStatusMessage(TEXT("Select a worker to repair or assist construction."));
            return false;
        }
        return SubmitNetworkCommandBatch(MoveTemp(Intents),
            bConstructionAssist ? TEXT("ASSIST CONSTRUCTION") : TEXT("REPAIR"),
            NetworkSimToWorld(Target->position), EEchoesCommandMarkerType::Interact);
    }
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!Bridge || !Bridge->IsScenarioReady()) return false;
    int32 Accepted = 0;
    int32 Rejected = 0;
    FString Refusal;
    for (const uint32 Id : SelectedEntityIds)
    {
        const auto* Worker = Bridge->FindEntity(Id);
        if (!Worker || Worker->type != EntityType::Worker) continue;
        FString Feedback;
        const TOptional<uint64> Before = Bridge->GetLastAcceptedLocalCommandSequence();
        const bool bQueued = bConstructionAssist
            ? Bridge->IssueConstructionAssistCommand(Id, TargetId, Feedback)
            : Bridge->IssueRepairCommand(Id, TargetId, Feedback);
        if (bQueued)
        {
            ++Accepted;
            CaptureTutorialAcceptedCommand(Bridge, Before,
                EEchoesTutorialOrderCommandOrigin::ContextAction);
        }
        else
        {
            ++Rejected;
            if (Refusal.IsEmpty()) Refusal = Feedback;
        }
    }
    if (Accepted > 0)
    {
        SetStatusMessage(FString::Printf(TEXT("%s ordered for %d worker%s.%s"),
            bConstructionAssist ? TEXT("Construction assist") : TEXT("Repair"),
            Accepted, Accepted == 1 ? TEXT("") : TEXT("s"),
            Rejected > 0 ? *FString::Printf(TEXT(" %d unavailable. %s"), Rejected, *Refusal) : TEXT("")));
        if (const auto* Target = Bridge->FindEntity(TargetId))
            ShowAcceptedCommandMarker(Bridge->SimToWorld(Target->position), EEchoesCommandMarkerType::Interact, Accepted);
        return true;
    }
    SetStatusMessage(Refusal.IsEmpty()
        ? TEXT("Select a worker, then choose an unfinished structure or damaged allied target.") : Refusal);
    return false;
}

void AEchoesPlayerController::RepairAtCursor()
{
    if (IsReplayInputActive() || IsModalOverlayVisible()) return;
    FHitResult Hit;
    if (!TraceCommandTarget(Hit))
    {
        SetStatusMessage(TEXT("Point at a damaged allied target to repair it."));
        return;
    }
    FVector2D Screen;
    const AEchoesEntityView* Target = ResolveCommandScreenPosition(!bKeyboardTargetingEnabled, Screen)
        ? TraceEntityUnderCommandTarget(Screen) : nullptr;
    if (!Target) Target = Cast<AEchoesEntityView>(Hit.GetActor());
    (void)IssueSelectedWorkerMaintenance(Target ? Target->GetEntityId() : 0, false);
}

void AEchoesPlayerController::RepairOrRestartPressed()
{
    if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift)) return;
    if (PlayerFlow.Is(EEchoesShellScreen::Results) || IsOnlineMatchResult())
        RestartScenario();
    else if (PlayerFlow.Is(EEchoesShellScreen::Gameplay) && !IsModalOverlayVisible())
        RepairAtCursor();
}

void AEchoesPlayerController::CancelSelectedConstruction()
{
    if (IsReplayInputActive() || IsModalOverlayVisible()) return;
    PruneSelection();
    if (SelectedEntityIds.Num() != 1)
    {
        SetStatusMessage(TEXT("Select one unfinished structure to cancel its construction."));
        return;
    }
    const uint32 SiteId = SelectedEntityIds[0];
    if (GetNetMode() == NM_Client)
    {
        const auto* Site = FindNetworkEntity(SiteId);
        if (!Site || Site->owner != NetworkSeat || Site->completed || !IsMaintenanceStructure(Site->type))
        {
            SetStatusMessage(TEXT("Only an owned unfinished structure can be cancelled."));
            return;
        }
        echoes::sim::net::CommandIntent Intent;
        Intent.type = echoes::sim::CommandType::CancelConstruction;
        Intent.actor = SiteId;
        Intent.position = Site->position;
        (void)SubmitNetworkCommandBatch({Intent}, TEXT("CANCEL CONSTRUCTION"),
            NetworkSimToWorld(Site->position), EEchoesCommandMarkerType::Interact);
        return;
    }
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!Bridge) return;
    FString Feedback;
    if (Bridge->IssueConstructionCancellation(SiteId, Feedback))
        SetStatusMessage(TEXT("Construction cancellation ordered. The refund follows progress when the order executes."));
    else
        SetStatusMessage(Feedback);
}
