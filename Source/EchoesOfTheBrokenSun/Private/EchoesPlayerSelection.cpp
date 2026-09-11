// Author: Angelis Pseftis
//
// Selection, control groups and subgroup cycling for AEchoesPlayerController.
// Bodies moved verbatim from EchoesPlayerController.cpp; no logic changed.

#include "EchoesPlayerController.h"
#include "EchoesInputPrompt.h"
#include "EchoesCheckpointFeedback.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesCinematicSubsystem.h"
#include "EchoesShellWidget.h"
#include "EchoesFieldHudWidget.h"
#include "EchoesPowerNetworkView.h"
#include "EchoesAmbienceSubsystem.h"
#include "EchoesCollisionChannels.h"
#include "EchoesCommandDeckLayout.h"
#include "EchoesCommandMarkerView.h"
#include "EchoesContextOrderReport.h"
#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesFactionPolicy.h"
#include "EchoesGameMode.h"
#include "EchoesGameInstance.h"
#include "EchoesGameUserSettings.h"
#include "EchoesHudLayout.h"
#include "EchoesInterfaceAudioSubsystem.h"
#include "EchoesMusicSubsystem.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesNetworkSession.h"
#include "EchoesOnlineFrontDoorLayout.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPresentationAudioSubsystem.h"
#include "EchoesPointerCombatGuardReview.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishOverlayLayout.h"
#include "EchoesTitleOverlayLayout.h"
#include "EchoesCampaignMapLayout.h"
#include "EchoesCampaignRewards.h"
#include "EchoesTechnologyPanelLayout.h"
#include "EchoesTerrainView.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/NetConnection.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/InputSettings.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"
#include <algorithm>
#include <limits>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
constexpr int32 ControlGroupCount = 10;
constexpr double ControlGroupDoubleTapSeconds = 0.300;
// Also defined in EchoesPlayerController.cpp, which retains IsDraggingSelection().
constexpr float DragSelectionThresholdPixels = 8.0f;
}

void AEchoesPlayerController::CycleOwnedEntityNext()
{
    // Tab is intentionally shared with CyclePlayableFaction. This field-only
    // action owns the selection behavior once the title/briefing is closed.
    if (PlayerFlow.Is(EEchoesShellScreen::Title) || PlayerFlow.Is(EEchoesShellScreen::Briefing))
    {
        return;
    }
    CycleSelectionSubgroupOrOwned(false);
}

void AEchoesPlayerController::CycleOwnedEntityPrevious()
{
    if (PlayerFlow.Is(EEchoesShellScreen::Title) || PlayerFlow.Is(EEchoesShellScreen::Briefing))
    {
        CyclePlayableFaction();
        return;
    }
    // Backspace steps through owned entities. It is deliberately not the
    // subgroup walk: SPEC-CTL-011 gives that to Shift+Tab, and folding both
    // into one command left a mixed selection unable to reach a single entity.
    CycleOwnedEntity(-1);
}

void AEchoesPlayerController::CycleSelectionSubgroupPrevious()
{
    // The Shift+Tab half of SPEC-CTL-011. It mirrors CycleOwnedEntityNext and
    // stays out of the title and briefing, where Tab cycles the faction.
    if (PlayerFlow.Is(EEchoesShellScreen::Title) || PlayerFlow.Is(EEchoesShellScreen::Briefing))
    {
        return;
    }
    CycleSelectionSubgroupOrOwned(true);
}

void AEchoesPlayerController::SelectCombatForce()
{
    if (IsReplayInputActive()) return;
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Combat-force selection is unavailable."));
        return;
    }

    TArray<uint32> CombatIds;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        const bool bCombatUnit =
            Entity.type == echoes::sim::EntityType::Soldier ||
            Entity.type == echoes::sim::EntityType::HeavyUnit ||
            Entity.type == echoes::sim::EntityType::ScoutUnit;
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.hitPoints > 0 && bCombatUnit &&
            !Entity.temporaryMineralCover &&
            Bridge->FindEntityView(Entity.id) != nullptr)
        {
            CombatIds.Add(Entity.id);
        }
    }
    CombatIds.Sort();
    if (CombatIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_COMBAT_FORCE] No live owned combat unit is visible."));
        return;
    }

    ClearSelection();
    for (const uint32 EntityId : CombatIds)
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }
    SetStatusMessage(
        FString::Printf(
            TEXT("COMBAT FORCE: %d visible owned units selected // End centers force"),
            CombatIds.Num()),
        5.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_FORCE_SELECT] count=%d source=owned_presentation_views hiddenStateRead=false"),
        CombatIds.Num());
}

void AEchoesPlayerController::CycleOwnedEntity(int32 Direction)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Keyboard selection is unavailable."));
        return;
    }

    TArray<uint32> Candidates;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.hitPoints > 0 && !Entity.temporaryMineralCover &&
            Bridge->FindEntityView(Entity.id) != nullptr)
        {
            Candidates.Add(Entity.id);
        }
    }
    Candidates.Sort();
    if (Candidates.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_OWNED_ENTITIES] No live owned entity can be selected."));
        return;
    }

    int32 CandidateIndex = Direction < 0 ? Candidates.Num() - 1 : 0;
    if (SelectedEntityIds.Num() == 1)
    {
        const int32 CurrentIndex = Candidates.IndexOfByKey(SelectedEntityIds[0]);
        if (CurrentIndex != INDEX_NONE)
        {
            CandidateIndex =
                (CurrentIndex + (Direction < 0 ? -1 : 1) + Candidates.Num()) %
                Candidates.Num();
        }
    }

    ClearSelection();
    const uint32 SelectedId = Candidates[CandidateIndex];
    SelectedEntityIds.Add(SelectedId);
    SetEntitySelected(SelectedId, true);
    const AEchoesEntityView* View = Bridge->FindEntityView(SelectedId);
    SetStatusMessage(
        FString::Printf(
            TEXT("KEYBOARD SELECT: %s  //  entity %u  //  Tab next / Backspace previous"),
            View != nullptr ? *View->GetDisplayName() : TEXT("owned entity"),
            SelectedId),
        4.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_SELECTION] entity=%u index=%d total=%d direction=%s owned=true"),
        SelectedId,
        CandidateIndex,
        Candidates.Num(),
        Direction < 0 ? TEXT("previous") : TEXT("next"));
}

void AEchoesPlayerController::SelectionReleased()
{
    if (bMinimapDragging) { bMinimapDragging = false; bSelectionButtonDown = false; return; }
    if (IsModalOverlayVisible())
    {
        bSelectionButtonDown = false;
        return;
    }
    if (!bSelectionButtonDown)
    {
        return;
    }

    FVector2D PointerPosition = SelectionCurrentScreenPosition;
    if (ResolvePointerScreenPosition(PointerPosition))
    {
        SelectionCurrentScreenPosition = PointerPosition;
    }
    bSelectionButtonDown = false;

    const bool bAdditive = IsInputKeyDown(EKeys::LeftShift) ||
                           IsInputKeyDown(EKeys::RightShift);
    if (FVector2D::Distance(
            SelectionStartScreenPosition,
            SelectionCurrentScreenPosition) >= DragSelectionThresholdPixels)
    {
        SelectInScreenRectangle(bAdditive);
    }
    else
    {
        SelectAtCursor(bAdditive);
    }
}

void AEchoesPlayerController::SelectAtCursor(bool bAdditive)
{
    if (IsReplayInputActive()) return;
    FHitResult HitResult;
    AEchoesEntityView* View = nullptr;
    if (TraceCursor(HitResult))
    {
        View = Cast<AEchoesEntityView>(HitResult.GetActor());
    }

    const uint8 SelectableOwner =
        GetNetMode() == NM_Client ? NetworkSeat
                                  : UEchoesSimulationSubsystem::LocalPlayerId;
    if (View == nullptr || View->GetOwnerPlayerId() != SelectableOwner)
    {
        if (!bAdditive)
        {
            ClearSelection();
            if (View != nullptr &&
                View->GetEntityType() == echoes::sim::EntityType::ResourceNode)
            {
                InspectDeposit(*View);
            }
            else if (View == nullptr && HitResult.bBlockingHit)
            {
                ObserveTutorialSelection(0, true);
                ObserveTutorialSelectionEvent(
                    EEchoesTutorialSelectionInputEvent::TerrainClear);
            }
        }
        return;
    }

    const uint32 EntityId = View->GetEntityId();
    if (!bAdditive)
    {
        ClearSelection();
    }
    // An owned selection replaces any deposit under inspection.
    InspectedEntityId = 0;

    if (bAdditive && SelectedEntityIds.Contains(EntityId))
    {
        SetEntitySelected(EntityId, false);
        SelectedEntityIds.Remove(EntityId);
    }
    else if (!SelectedEntityIds.Contains(EntityId))
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }
    NormalizeSelectionSubgroup();
    ObserveTutorialSelection(EntityId, false);
    ObserveTutorialSelectionEvent(
        bAdditive
            ? EEchoesTutorialSelectionInputEvent::SelectionModified
            : EEchoesTutorialSelectionInputEvent::SingleClickSelection);

    SetStatusMessage(
        FString::Printf(
            TEXT("Selected %d owned entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_POINTER_SELECTION] screen=(%.1f,%.1f) entity=%u selected=%d additive=%s ownerScoped=true"),
        LastPointerScreenPosition.X,
        LastPointerScreenPosition.Y,
        EntityId,
        SelectedEntityIds.Num(),
        bAdditive ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::InspectDeposit(const AEchoesEntityView& View)
{
    // SPEC-RES-006.INSPECT (owner ruling 2026-09-11): a click on a visible
    // deposit answers "how much is left" without selecting it. The stock is
    // read from the local simulation; a network client sees only what its
    // scoped keyframe carries, which is no stock today.
    InspectedEntityId = View.GetEntityId();
    int32 Remaining = -1;
    if (GetNetMode() != NM_Client)
    {
        const UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        const echoes::sim::Entity* Deposit =
            Bridge != nullptr ? Bridge->FindEntity(InspectedEntityId) : nullptr;
        if (Deposit != nullptr &&
            Deposit->type == echoes::sim::EntityType::ResourceNode)
        {
            Remaining = Deposit->resourceRemaining;
        }
    }
    SetStatusMessage(
        Remaining < 0
            ? FString(TEXT("Matter deposit: stock unknown from here."))
            : Remaining == 0
                ? FString(TEXT("Matter deposit: exhausted. Send Surveyors to another known deposit."))
                : FString::Printf(
                    TEXT("Matter deposit: %s Matter remaining."),
                    *FText::AsNumber(Remaining).ToString()),
        4.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_POINTER_INSPECTION] screen=(%.1f,%.1f) entity=%u remaining=%d"),
        LastPointerScreenPosition.X,
        LastPointerScreenPosition.Y,
        InspectedEntityId,
        Remaining);
}

void AEchoesPlayerController::SelectInScreenRectangle(bool bAdditive)
{
    if (IsReplayInputActive()) return;
    if (GetNetMode() == NM_Client)
    {
        const echoes::sim::net::ScopedViewKeyframe* NetworkView =
            GetNetworkScopedView();
        if (!IsNetworkClientControlActive() || NetworkView == nullptr)
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Drag selection is unavailable."));
            return;
        }
        if (!bAdditive)
        {
            ClearSelection();
        }
        const float MinX = FMath::Min(
            SelectionStartScreenPosition.X,
            SelectionCurrentScreenPosition.X);
        const float MaxX = FMath::Max(
            SelectionStartScreenPosition.X,
            SelectionCurrentScreenPosition.X);
        const float MinY = FMath::Min(
            SelectionStartScreenPosition.Y,
            SelectionCurrentScreenPosition.Y);
        const float MaxY = FMath::Max(
            SelectionStartScreenPosition.Y,
            SelectionCurrentScreenPosition.Y);
        for (const echoes::sim::net::ScopedEntityState& Entity :
             NetworkView->entities)
        {
            if (Entity.owner != NetworkSeat)
            {
                continue;
            }
            const TWeakObjectPtr<AEchoesEntityView>* StoredView =
                NetworkEntityViews.Find(Entity.id);
            AEchoesEntityView* EntityView =
                StoredView != nullptr ? StoredView->Get() : nullptr;
            if (EntityView == nullptr)
            {
                continue;
            }
            FVector2D ScreenPosition;
            if (ProjectWorldLocationToScreen(
                    EntityView->GetActorLocation() +
                        FVector(0.0f, 0.0f, 60.0f),
                    ScreenPosition,
                    false) &&
                ScreenPosition.X >= MinX && ScreenPosition.X <= MaxX &&
                ScreenPosition.Y >= MinY && ScreenPosition.Y <= MaxY &&
                !SelectedEntityIds.Contains(Entity.id))
            {
                SelectedEntityIds.Add(Entity.id);
                EntityView->SetSelected(true);
            }
        }
        SetStatusMessage(
            FString::Printf(
                TEXT("ONLINE DRAG SELECT: %d owned entit%s."),
                SelectedEntityIds.Num(),
                SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
            2.0f);
        NormalizeSelectionSubgroup();
        ObserveTutorialSelectionEvent(
            bAdditive
                ? EEchoesTutorialSelectionInputEvent::SelectionModified
                : EEchoesTutorialSelectionInputEvent::DragSelection);
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Sim =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Sim == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Drag selection is unavailable."));
        return;
    }

    if (!bAdditive)
    {
        ClearSelection();
    }

    const float MinX = FMath::Min(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MaxX = FMath::Max(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MinY = FMath::Min(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);
    const float MaxY = FMath::Max(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);

    for (const echoes::sim::Entity& Entity : Sim->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        AEchoesEntityView* View = Bridge->FindEntityView(Entity.id);
        if (View == nullptr)
        {
            continue;
        }

        FVector2D ScreenPosition;
        if (ProjectWorldLocationToScreen(
                View->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f),
                ScreenPosition,
                false) &&
            ScreenPosition.X >= MinX && ScreenPosition.X <= MaxX &&
            ScreenPosition.Y >= MinY && ScreenPosition.Y <= MaxY &&
            !SelectedEntityIds.Contains(Entity.id))
        {
            SelectedEntityIds.Add(Entity.id);
            View->SetSelected(true);
        }
    }

    SetStatusMessage(
        FString::Printf(
            TEXT("Drag-selected %d owned entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
    NormalizeSelectionSubgroup();
    ObserveTutorialSelectionEvent(
        bAdditive
            ? EEchoesTutorialSelectionInputEvent::SelectionModified
            : EEchoesTutorialSelectionInputEvent::DragSelection);
}

void AEchoesPlayerController::NormalizeSelectionSubgroup()
{
    const TArray<echoes::sim::EntityType> Types = GetSelectionSubgroupTypes();
    if (Types.IsEmpty())
    {
        ActiveSelectionSubgroupIndex = INDEX_NONE;
        return;
    }
    if (!Types.IsValidIndex(ActiveSelectionSubgroupIndex))
    {
        ActiveSelectionSubgroupIndex = 0;
    }
}

void AEchoesPlayerController::CycleSelectionSubgroupOrOwned(bool bPrevious)
{
    if (IsReplayInputActive() || IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetSelectionSubgroupTypes().Num() >= 2)
    {
        CycleSelectionSubgroup(bPrevious);
        return;
    }
    CycleOwnedEntity(bPrevious ? -1 : 1);
}

void AEchoesPlayerController::CycleSelectionSubgroup(bool bPrevious)
{
    if (IsReplayInputActive() || IsModalOverlayVisible()) return;
    PruneSelection();
    const TArray<echoes::sim::EntityType> Types = GetSelectionSubgroupTypes();
    if (Types.Num() < 2)
    {
        NormalizeSelectionSubgroup();
        SetStatusMessage(
            TEXT("[SUBGROUP_UNAVAILABLE] Select a mixed force to cycle subgroups."),
            2.0f);
        return;
    }
    if (!Types.IsValidIndex(ActiveSelectionSubgroupIndex))
    {
        ActiveSelectionSubgroupIndex = 0;
    }
    const echoes::sim::EntityType Previous =
        Types[ActiveSelectionSubgroupIndex];
    ActiveSelectionSubgroupIndex =
        (ActiveSelectionSubgroupIndex + (bPrevious ? Types.Num() - 1 : 1)) %
        Types.Num();
    const echoes::sim::EntityType Active =
        Types[ActiveSelectionSubgroupIndex];
    ObserveTutorialSelectionEvent(
        EEchoesTutorialSelectionInputEvent::SubgroupChanged,
        INDEX_NONE,
        Previous,
        Active);
    SetStatusMessage(FString::Printf(
        TEXT("ACTIVE SUBGROUP %d OF %d."),
        ActiveSelectionSubgroupIndex + 1,
        Types.Num()), 2.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_SELECTION_SUBGROUP] direction=%s previousType=%d activeType=%d subgroup=%d total=%d"),
        bPrevious ? TEXT("previous") : TEXT("next"),
        static_cast<int32>(Previous),
        static_cast<int32>(Active),
        ActiveSelectionSubgroupIndex,
        Types.Num());
}

bool AEchoesPlayerController::SetControlGroup(
    int32 GroupIndex,
    const TArray<uint32>& EntityIds,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (GroupIndex < 0 || GroupIndex >= ControlGroupCount)
    {
        OutFeedback = TEXT("[GROUP_INDEX_INVALID] Control group must be between 0 and 9.");
        return false;
    }
    if (EntityIds.IsEmpty())
    {
        ControlGroups[GroupIndex].Reset();
        OutFeedback = FString::Printf(
            TEXT("CONTROL GROUP %d CLEARED."),
            ControlGroupDisplayNumber(GroupIndex));
        return true;
    }

    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    TArray<uint32> ValidIds;
    for (const uint32 EntityId : EntityIds)
    {
        const echoes::sim::net::ScopedEntityState* NetworkEntity =
            GetNetMode() == NM_Client ? FindNetworkEntity(EntityId) : nullptr;
        const echoes::sim::Entity* Entity =
            GetNetMode() != NM_Client && Bridge != nullptr
                ? Bridge->FindEntity(EntityId)
                : nullptr;
        if ((NetworkEntity != nullptr && NetworkEntity->owner == NetworkSeat) ||
            (Entity != nullptr &&
             Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId))
        {
            ValidIds.AddUnique(EntityId);
        }
    }
    if (ValidIds.IsEmpty())
    {
        OutFeedback = TEXT("[GROUP_NO_VALID_ENTITIES] No live local entities were assigned.");
        return false;
    }
    ValidIds.Sort();
    ControlGroups[GroupIndex] = MoveTemp(ValidIds);
    OutFeedback = FString::Printf(
        TEXT("CONTROL GROUP %d: %d entit%s assigned."),
        ControlGroupDisplayNumber(GroupIndex),
        ControlGroups[GroupIndex].Num(),
        ControlGroups[GroupIndex].Num() == 1 ? TEXT("y") : TEXT("ies"));
    return true;
}

TArray<uint32> AEchoesPlayerController::GetValidControlGroup(
    int32 GroupIndex) const
{
    TArray<uint32> ValidIds;
    if (GroupIndex < 0 || GroupIndex >= ControlGroupCount)
    {
        return ValidIds;
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    for (const uint32 EntityId : ControlGroups[GroupIndex])
    {
        const echoes::sim::net::ScopedEntityState* NetworkEntity =
            GetNetMode() == NM_Client ? FindNetworkEntity(EntityId) : nullptr;
        const echoes::sim::Entity* Entity =
            GetNetMode() != NM_Client && Bridge != nullptr
                ? Bridge->FindEntity(EntityId)
                : nullptr;
        if ((NetworkEntity != nullptr && NetworkEntity->owner == NetworkSeat) ||
            (Entity != nullptr &&
             Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId))
        {
            ValidIds.Add(EntityId);
        }
    }
    return ValidIds;
}

int32 AEchoesPlayerController::ControlGroupDisplayNumber(int32 GroupIndex)
{
    return GroupIndex == ControlGroupCount - 1 ? 0 : GroupIndex + 1;
}

void AEchoesPlayerController::ClearControlGroups()
{
    for (int32 GroupIndex = 0; GroupIndex < ControlGroupCount; ++GroupIndex)
    {
        ControlGroups[GroupIndex].Reset();
        LastControlGroupRecallRealTime[GroupIndex] = 0.0;
    }
}

void AEchoesPlayerController::AssignControlGroupFromSelection(int32 GroupIndex)
{
    if (IsReplayInputActive()) return;
    PruneSelection();
    FString Feedback;
    if (SetControlGroup(GroupIndex, SelectedEntityIds, Feedback))
    {
        ObserveTutorialSelectionEvent(
            EEchoesTutorialSelectionInputEvent::ControlGroupAssigned,
            GroupIndex);
    }
    SetStatusMessage(Feedback);
}

void AEchoesPlayerController::ArmControlGroupAssignment()
{
    if (IsReplayInputActive()) return;
    if (IsModalOverlayVisible())
    {
        return;
    }
    bControlGroupAssignmentArmed = true;
    ControlGroupAssignmentExpiresAt =
        GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + 5.0 : 5.0;
    SetStatusMessage(
        TEXT("GROUP ASSIGNMENT ARMED — press 1-0 within five seconds."),
        5.0f);
}

void AEchoesPlayerController::RecallControlGroup(int32 GroupIndex)
{
    if (IsReplayInputActive()) return;
    if (IsModalOverlayVisible())
    {
        return;
    }
    const bool bControlDown =
        IsInputKeyDown(EKeys::LeftControl) ||
        IsInputKeyDown(EKeys::RightControl);
    const bool bShiftDown =
        IsInputKeyDown(EKeys::LeftShift) ||
        IsInputKeyDown(EKeys::RightShift);
    if (bControlGroupAssignmentArmed || bControlDown)
    {
        bControlGroupAssignmentArmed = false;
        if (!bShiftDown)
        {
            AssignControlGroupFromSelection(GroupIndex);
            return;
        }

        PruneSelection();
        TArray<uint32> Combined = GetValidControlGroup(GroupIndex);
        for (const uint32 EntityId : SelectedEntityIds)
        {
            Combined.AddUnique(EntityId);
        }
        FString Feedback;
        if (SetControlGroup(GroupIndex, Combined, Feedback))
        {
            ObserveTutorialSelectionEvent(
                EEchoesTutorialSelectionInputEvent::ControlGroupAssigned,
                GroupIndex);
        }
        SetStatusMessage(Feedback);
        return;
    }
    TArray<uint32> ValidIds = GetValidControlGroup(GroupIndex);
    if (ValidIds.IsEmpty())
    {
        ControlGroups[GroupIndex].Reset();
        SetStatusMessage(FString::Printf(
            TEXT("[GROUP_EMPTY] Control group %d has no live entities."),
            ControlGroupDisplayNumber(GroupIndex)));
        return;
    }
    ControlGroups[GroupIndex] = ValidIds;
    if (!bShiftDown)
    {
        ClearSelection();
    }
    for (const uint32 EntityId : ValidIds)
    {
        SelectedEntityIds.AddUnique(EntityId);
        SetEntitySelected(EntityId, true);
    }
    NormalizeSelectionSubgroup();
    ObserveTutorialSelectionEvent(
        EEchoesTutorialSelectionInputEvent::ControlGroupRecalled,
        GroupIndex);

    bool bCentered = false;
    if (!bShiftDown && GetWorld() != nullptr)
    {
        const double Now = GetWorld()->GetRealTimeSeconds();
        const double Previous = LastControlGroupRecallRealTime[GroupIndex];
        bCentered = Previous > 0.0 && Now >= Previous &&
            Now - Previous <= ControlGroupDoubleTapSeconds;
        LastControlGroupRecallRealTime[GroupIndex] = bCentered ? 0.0 : Now;
        if (bCentered)
        {
            SnapKeyboardTargetToSelection();
        }
    }
    SetStatusMessage(FString::Printf(
        TEXT("CONTROL GROUP %d: %d entit%s selected.%s"),
        ControlGroupDisplayNumber(GroupIndex),
        ValidIds.Num(),
        ValidIds.Num() == 1 ? TEXT("y") : TEXT("ies"),
        bCentered ? TEXT(" Camera centered.") : TEXT("")));
}
