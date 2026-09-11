// Author: Angelis Pseftis
//
// Order issuing for AEchoesPlayerController. Bodies moved verbatim from
// EchoesPlayerController.cpp; no logic changed.

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

void AEchoesPlayerController::IssueContextOrder(
    const FHitResult& HitResult,
    bool bPointerSource)
{
    // Shift appends to the actor's order queue instead of replacing it
    // (Entity::orderQueue, kMaxQueuedOrders = 16).
    const bool bQueueOrder = IsInputKeyDown(EKeys::LeftShift) ||
        IsInputKeyDown(EKeys::RightShift);
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    // Two questions, two answers. HitResult stays the ECC_Visibility ground
    // hit and keeps supplying Destination; the entity under the cursor comes
    // from the dedicated pick channel, which the ground plane and the terrain
    // ignore. The ground trace's own actor is still honoured, so a click that
    // lands directly on a body resolves the same entity it always did even
    // before the pick geometry is reached.
    FVector2D CommandScreenPosition = FVector2D::ZeroVector;
    const AEchoesEntityView* TargetView =
        ResolveCommandScreenPosition(bPointerSource, CommandScreenPosition)
            ? TraceEntityUnderCommandTarget(CommandScreenPosition)
            : nullptr;
    if (TargetView == nullptr)
    {
        TargetView = Cast<AEchoesEntityView>(HitResult.GetActor());
    }
    if (TargetView != nullptr && TryIssueWorkerMaintenanceContext(TargetView->GetEntityId()))
    {
        return;
    }
    if (GetNetMode() == NM_Client)
    {
        const echoes::sim::net::ScopedEntityState* TargetEntity =
            TargetView != nullptr
                ? FindNetworkEntity(TargetView->GetEntityId())
                : nullptr;
        echoes::sim::CommandType CommandType =
            echoes::sim::CommandType::Move;
        uint32 TargetId = 0;
        FVector Destination = HitResult.Location;
        if (TargetEntity != nullptr)
        {
            TargetId = TargetEntity->id;
            Destination = NetworkSimToWorld(TargetEntity->position);
            if (TargetEntity->type == echoes::sim::EntityType::ResourceNode)
            {
                CommandType = echoes::sim::CommandType::Gather;
            }
            else if (TargetEntity->type == echoes::sim::EntityType::FutureWell)
            {
                CommandType = echoes::sim::CommandType::FutureWell;
            }
            else if (TargetEntity->owner != echoes::sim::kNeutralPlayer &&
                     TargetEntity->owner != NetworkSeat)
            {
                CommandType = echoes::sim::CommandType::Attack;
            }
            else if (TargetEntity->owner == NetworkSeat &&
                     (TargetEntity->type ==
                          echoes::sim::EntityType::CommandCore ||
                      TargetEntity->type == echoes::sim::EntityType::Dropoff))
            {
                CommandType = echoes::sim::CommandType::Deliver;
            }
        }
        const TArray<FVector> FormationDestinations =
            BuildSelectedFormationDestinations(
                Destination, SelectedEntityIds.Num());
        TArray<echoes::sim::net::CommandIntent> Intents;
        Intents.Reserve(SelectedEntityIds.Num());
        for (int32 Index = 0; Index < SelectedEntityIds.Num(); ++Index)
        {
            const echoes::sim::net::ScopedEntityState* Actor =
                FindNetworkEntity(SelectedEntityIds[Index]);
            if (Actor == nullptr || Actor->owner != NetworkSeat)
            {
                continue;
            }
            echoes::sim::net::CommandIntent Intent{};
            Intent.type = CommandType;
            Intent.actor = Actor->id;
            Intent.target = TargetId;
            Intent.position = NetworkWorldToSim(
                CommandType == echoes::sim::CommandType::Move
                    ? FormationDestinations[Index]
                    : Destination);
            Intent.wellChoice = TargetEntity != nullptr &&
                    TargetEntity->type == echoes::sim::EntityType::FutureWell &&
                    TargetEntity->wellChoice == echoes::sim::FutureWellChoice::Preserve
                ? echoes::sim::FutureWellChoice::Preserve : FutureWellChoice;
            Intents.Add(Intent);
        }
        const FString OrderLabel =
            CommandType == echoes::sim::CommandType::Move
                ? FString::Printf(TEXT("ONLINE MOVE / %s"), *GetFormationLabel())
                : FString::Printf(
                      TEXT("ONLINE %s"), *CommandLabel(CommandType));
        const EEchoesCommandMarkerType MarkerType =
            CommandType == echoes::sim::CommandType::Attack
                ? EEchoesCommandMarkerType::Attack
            : CommandType == echoes::sim::CommandType::Move
                ? EEchoesCommandMarkerType::Move
                : EEchoesCommandMarkerType::Interact;
        (void)SubmitNetworkCommandBatch(
            MoveTemp(Intents), OrderLabel, Destination, MarkerType);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_CONTEXT_ORDER] source=%s command=%s target=%u selected=%d visibleHit=%s"),
            bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
            *CommandLabel(CommandType),
            TargetId,
            SelectedEntityIds.Num(),
            TargetView != nullptr ? TEXT("true") : TEXT("false"));
        return;
    }
    if (Bridge == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (bTutorialOperationAuthorized && (GetTutorialGateMask() & 2) == 0)
    {
        SetStatusMessage(TEXT("[TUTORIAL] Follow the active tutorial step before issuing orders."));
        return;
    }
    SynchronizeBoundCampaignProtocol();

    const echoes::sim::Entity* TargetEntity =
        TargetView != nullptr
            ? Bridge->FindEntity(TargetView->GetEntityId())
            : nullptr;

    // A single selected local producer gives context order its authored rally
    // meaning. The bridge validates ground, allied Guard, and resource Gather
    // targets; the controller does not infer an order for the future unit.
    if (SelectedEntityIds.Num() == 1)
    {
        echoes::sim::ProducerQueueState ProducerState;
        const uint32 ProducerId = SelectedEntityIds[0];
        if (Bridge->GetLocalProducerQueueState(ProducerId, ProducerState))
        {
            const uint32 RallyTargetId =
                TargetEntity != nullptr ? TargetEntity->id : 0;
            const FVector RallyDestination = TargetEntity != nullptr
                ? Bridge->SimToWorld(TargetEntity->position)
                : FVector(HitResult.Location);
            const bool bAppend = IsInputKeyDown(EKeys::LeftShift) ||
                IsInputKeyDown(EKeys::RightShift);
            FString Feedback;
            if (Bridge->IssueRallyCommand(
                    ProducerId,
                    RallyTargetId,
                    RallyDestination,
                    bAppend,
                    Feedback))
            {
                SetStatusMessage(
                    bAppend
                        ? NSLOCTEXT(
                              "EchoesPlayer", "RallyExtended", "Rally route extended.")
                              .ToString()
                        : NSLOCTEXT(
                              "EchoesPlayer", "RallySet", "Rally point set.")
                              .ToString());
                ShowAcceptedCommandMarker(
                    RallyDestination,
                    RallyTargetId == 0
                        ? EEchoesCommandMarkerType::Move
                        : EEchoesCommandMarkerType::Interact,
                    1);
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_RALLY_COMMAND] source=%s producer=%u target=%u append=%s"),
                    bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
                    ProducerId,
                    RallyTargetId,
                    bAppend ? TEXT("true") : TEXT("false"));
            }
            else
            {
                SetStatusMessage(NSLOCTEXT(
                    "EchoesPlayer", "RallyRefused", "Choose open ground, an allied unit, or a Matter node.")
                    .ToString());
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_RALLY_COMMAND_REFUSED] source=%s producer=%u target=%u reason=%s"),
                    bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
                    ProducerId,
                    RallyTargetId,
                    *Feedback);
            }
            return;
        }
    }

    echoes::sim::CommandType CommandType = echoes::sim::CommandType::Move;
    uint32 TargetId = 0;
    FVector Destination = HitResult.Location;
    if (TargetEntity != nullptr)
    {
        TargetId = TargetEntity->id;
        Destination = Bridge->SimToWorld(TargetEntity->position);
        if (TargetEntity->type == echoes::sim::EntityType::ResourceNode)
        {
            CommandType = echoes::sim::CommandType::Gather;
        }
        else if (TargetEntity->type == echoes::sim::EntityType::FutureWell)
        {
            CommandType = echoes::sim::CommandType::FutureWell;
        }
        else if (Bridge->GetSimulation() != nullptr &&
                 Bridge->GetSimulation()->Config().IsHostile(
                     UEchoesSimulationSubsystem::LocalPlayerId, TargetEntity->owner))
        {
            CommandType = echoes::sim::CommandType::Attack;
        }
        else if (TargetEntity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 (TargetEntity->type == echoes::sim::EntityType::CommandCore ||
                  TargetEntity->type == echoes::sim::EntityType::Dropoff))
        {
            CommandType = echoes::sim::CommandType::Deliver;
        }
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(Destination, UnitCount);
    FEchoesContextOrderOutcome Outcome;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const uint32 ActorId = SelectedEntityIds[Index];
        echoes::sim::CommandType ActorCommandType = CommandType;
        uint32 ActorTargetId = TargetId;
        const echoes::sim::Entity* ActorState = Bridge->FindEntity(ActorId);
        // A Deliver is substituted with a move to the drop-off only when the
        // authoritative state says this unit cannot deliver: it is not a
        // worker, or its hold is empty. Each of those is recorded as the fact
        // it is, so the banner below never merges them into one claim.
        //
        // A unit whose state could not be read is NOT substituted. A failed
        // lookup establishes nothing about cargo, so the Deliver goes to the
        // authority unchanged and whatever the authority answers is what the
        // player is told.
        const bool bCannotCarry =
            CommandType == echoes::sim::CommandType::Deliver &&
            ActorState != nullptr &&
            ActorState->type != echoes::sim::EntityType::Worker;
        const bool bNotCarrying =
            CommandType == echoes::sim::CommandType::Deliver &&
            ActorState != nullptr &&
            ActorState->type == echoes::sim::EntityType::Worker &&
            ActorState->cargo <= 0;
        const bool bSubstitutedDropoffMove = bCannotCarry || bNotCarrying;
        if (bSubstitutedDropoffMove)
        {
            ActorCommandType = echoes::sim::CommandType::Move;
            ActorTargetId = 0;
        }

        FVector UnitDestination = Destination;
        if (ActorCommandType == echoes::sim::CommandType::Move)
        {
            UnitDestination = FormationDestinations[Index];
        }

        FString Feedback;
        const TOptional<uint64> SequenceBefore =
            Bridge->GetLastAcceptedLocalCommandSequence();
        if (Bridge->IssueCommand(
                ActorCommandType,
                ActorId,
                ActorTargetId,
                UnitDestination,
                TargetEntity != nullptr &&
                    TargetEntity->type == echoes::sim::EntityType::FutureWell &&
                    TargetEntity->wellChoice == echoes::sim::FutureWellChoice::Preserve
                        ? echoes::sim::FutureWellChoice::Preserve : FutureWellChoice,
                Feedback,
                bQueueOrder))
        {
            CaptureTutorialAcceptedCommand(
                Bridge,
                SequenceBefore,
                EEchoesTutorialOrderCommandOrigin::ContextAction);
            if (bCannotCarry)
            {
                ++Outcome.MovedCannotCarryCount;
            }
            else if (bNotCarrying)
            {
                ++Outcome.MovedNotCarryingCount;
            }
            else if (CommandType == echoes::sim::CommandType::Deliver)
            {
                ++Outcome.DeliveredCount;
            }
            else
            {
                ++Outcome.AcceptedOtherCount;
            }
        }
        else
        {
            FEchoesTutorialExpectedCommand RejectedAttempt;
            RejectedAttempt.Type = ActorCommandType;
            RejectedAttempt.Actor = ActorId;
            RejectedAttempt.Target = ActorTargetId;
            RejectedAttempt.Position = Bridge->WorldToSim(UnitDestination);
            (void)ObserveTutorialRejectedCommandAttempt(RejectedAttempt);
            Outcome.RecordRejection(ActorId, Feedback);
        }
    }

    const int32 AcceptedCount = Outcome.AcceptedCount();
    if (AcceptedCount > 0)
    {
        const FString OrderSummary =
            CommandType == echoes::sim::CommandType::Deliver
                ? FEchoesContextOrderReport::ComposeDeliverBanner(Outcome)
                : FEchoesContextOrderReport::ComposeOrderBanner(
                      CommandType == echoes::sim::CommandType::Move
                          ? FString::Printf(
                                TEXT("MOVE / %s"),
                                *GetFormationLabel())
                          : CommandLabel(CommandType),
                      Outcome);
        SetStatusMessage(OrderSummary);
        // A Deliver that produced no delivery is a move, and its ground marker
        // says so.
        const bool bGroundMoveMarker =
            CommandType == echoes::sim::CommandType::Move ||
            (CommandType == echoes::sim::CommandType::Deliver &&
             Outcome.DeliveredCount <= 0);
        ShowAcceptedCommandMarker(
            Destination,
            CommandType == echoes::sim::CommandType::Attack
                ? EEchoesCommandMarkerType::Attack
                : bGroundMoveMarker
                      ? EEchoesCommandMarkerType::Move
                      : EEchoesCommandMarkerType::Interact,
            AcceptedCount);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CONTEXT_ORDER_ACCEPTED] source=%s screen=(%.1f,%.1f) command=%s target=%u accepted=%d delivered=%d movedEmptyHold=%d movedCannotCarry=%d rejected=%d rejectedEntity=%u visibleHit=%s"),
            bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
            bPointerSource ? LastPointerScreenPosition.X : -1.0f,
            bPointerSource ? LastPointerScreenPosition.Y : -1.0f,
            *CommandLabel(CommandType),
            TargetId,
            AcceptedCount,
            Outcome.DeliveredCount,
            Outcome.MovedNotCarryingCount,
            Outcome.MovedCannotCarryCount,
            Outcome.RejectedCount,
            Outcome.RejectionEntityId,
            TargetView != nullptr ? TEXT("true") : TEXT("false"));
    }
    else
    {
        SetStatusMessage(Outcome.RejectionReason.IsEmpty()
                             ? TEXT("[ORDER_REJECTED] No selected entity accepted the order.")
                             : Outcome.RejectionReason);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CONTEXT_ORDER_REJECTED] source=%s screen=(%.1f,%.1f) command=%s target=%u rejected=%d rejectedEntity=%u reason=%s"),
            bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
            bPointerSource ? LastPointerScreenPosition.X : -1.0f,
            bPointerSource ? LastPointerScreenPosition.Y : -1.0f,
            *CommandLabel(CommandType),
            TargetId,
            Outcome.RejectedCount,
            Outcome.RejectionEntityId,
            Outcome.RejectionReason.IsEmpty() ? TEXT("ORDER_REJECTED")
                                              : *Outcome.RejectionReason);
    }
}

void AEchoesPlayerController::AttackMoveAtCursor()
{
    // Shift appends to the actor's order queue instead of replacing it
    // (Entity::orderQueue, kMaxQueuedOrders = 16).
    const bool bQueueOrder = IsInputKeyDown(EKeys::LeftShift) ||
        IsInputKeyDown(EKeys::RightShift);
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Attack-move requires an active remote battlefield target."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::AttackMove,
            0,
            HitResult.Location,
            true,
            false,
            FString::Printf(
                TEXT("ONLINE ATTACK-MOVE / %s"), *GetFormationLabel()),
            EEchoesCommandMarkerType::AttackMove);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Attack-move is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned combat units first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target an attack-move destination with the pointer or center reticle."));
        return;
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(HitResult.Location, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const FVector UnitDestination = FormationDestinations[Index];
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::AttackMove,
                SelectedEntityIds[Index],
                0,
                UnitDestination,
                FutureWellChoice,
                Feedback,
                bQueueOrder))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("ATTACK-MOVE / %s: %d queued%s"),
            *GetFormationLabel(),
            AcceptedCount,
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::AttackMove,
            AcceptedCount);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ATTACK_MOVE_ACCEPTED] source=%s screen=(%.1f,%.1f) accepted=%d rejected=%d formation=%s"),
            bKeyboardTargetingEnabled ? TEXT("keyboard_reticle") : TEXT("pointer"),
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.X,
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.Y,
            AcceptedCount,
            RejectedCount,
            *GetFormationLabel());
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[ATTACK_MOVE_REJECTED] No selected entity can attack-move.")
                : LastRejection);
    }
}

void AEchoesPlayerController::PatrolAtCursor()
{
    // Shift appends to the actor's order queue instead of replacing it
    // (Entity::orderQueue, kMaxQueuedOrders = 16).
    const bool bQueueOrder = IsInputKeyDown(EKeys::LeftShift) ||
        IsInputKeyDown(EKeys::RightShift);
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Patrol requires an active remote battlefield target."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Patrol,
            0,
            HitResult.Location,
            true,
            false,
            FString::Printf(
                TEXT("ONLINE PATROL / %s"), *GetFormationLabel()),
            EEchoesCommandMarkerType::Patrol);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Patrol is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned combat units first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target a patrol endpoint with the pointer or center reticle."));
        return;
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(HitResult.Location, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const FVector UnitDestination = FormationDestinations[Index];
        FString Feedback;
        const TOptional<uint64> SequenceBefore =
            Bridge->GetLastAcceptedLocalCommandSequence();
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::Patrol,
                SelectedEntityIds[Index],
                0,
                UnitDestination,
                FutureWellChoice,
                Feedback,
                bQueueOrder))
        {
            CaptureTutorialAcceptedCommand(
                Bridge,
                SequenceBefore,
                EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("PATROL / %s: %d route%s assigned%s"),
            *GetFormationLabel(),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::Patrol,
            AcceptedCount);
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[PATROL_REJECTED] No selected entity can patrol.")
                : LastRejection);
    }
}

void AEchoesPlayerController::StopSelectedUnits()
{
    // Shift+X belongs to explicit construction cancellation.
    if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift)) return;
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Stop is unavailable."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Stop,
            0,
            FVector::ZeroVector,
            false,
            true,
            TEXT("ONLINE STOP"),
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Stop is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned units first."));
        return;
    }
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    const bool bCancellingResearch =
        Player != nullptr &&
        Player->activeResearch != echoes::sim::ResearchType::None &&
        SelectedEntityIds.Contains(Player->researchProducer);
    const uint32 ResearchProducer =
        bCancellingResearch ? Player->researchProducer : 0;
    const echoes::sim::ResearchType InterruptedResearch =
        bCancellingResearch
            ? Player->activeResearch
            : echoes::sim::ResearchType::None;
    int32 AcceptedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        FString Feedback;
        const TOptional<uint64> SequenceBefore =
            Bridge->GetLastAcceptedLocalCommandSequence();
        if (Entity != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Stop,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            CaptureTutorialAcceptedCommand(
                Bridge,
                SequenceBefore,
                EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
            ++AcceptedCount;
        }
        else
        {
            LastRejection = Feedback;
        }
    }
    const FString StopFeedback =
        bCancellingResearch && AcceptedCount > 0
            ? TEXT("RESEARCH INTERRUPTION QUEUED: selected producer stopped // costs will not be refunded.")
            : AcceptedCount > 0
                  ? FString::Printf(
                        TEXT("STOP: %d unit%s stopped."),
                        AcceptedCount,
                        AcceptedCount == 1 ? TEXT("") : TEXT("s"))
                  : LastRejection.IsEmpty()
                        ? TEXT("[STOP_REJECTED] No selected entity accepted the order.")
                        : LastRejection;
    SetStatusMessage(StopFeedback);
    if (bCancellingResearch && AcceptedCount > 0)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_RESEARCH_CANCEL_QUEUED] player=%u producer=%u technology=%u costsRefunded=false input=stop"),
            UEchoesSimulationSubsystem::LocalPlayerId,
            ResearchProducer,
            static_cast<uint8>(InterruptedResearch));
    }
}

void AEchoesPlayerController::HoldSelectedUnits()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Hold position is unavailable."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Hold,
            0,
            FVector::ZeroVector,
            false,
            true,
            TEXT("ONLINE HOLD POSITION"),
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Hold position is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned defenders first."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        FString Feedback;
        if (Entity != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Hold,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("HOLD POSITION: %d defender%s anchored%s"),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            *RejectionSuffix));
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[HOLD_REJECTED] No selected entity can defend a position.")
                : LastRejection);
    }
}

void AEchoesPlayerController::GuardAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Guard requires a visible owned target."));
            return;
        }
        FVector2D GuardScreenPosition = FVector2D::ZeroVector;
        const AEchoesEntityView* TargetView =
            ResolveCommandScreenPosition(
                !bKeyboardTargetingEnabled, GuardScreenPosition)
                ? TraceEntityUnderCommandTarget(GuardScreenPosition)
                : nullptr;
        if (TargetView == nullptr)
        {
            TargetView = Cast<AEchoesEntityView>(HitResult.GetActor());
        }
        const echoes::sim::net::ScopedEntityState* Target =
            TargetView != nullptr
                ? FindNetworkEntity(TargetView->GetEntityId())
                : nullptr;
        if (Target == nullptr || Target->owner != NetworkSeat)
        {
            SetStatusMessage(TEXT("[GUARD_TARGET_INVALID] Point at a live owned entity."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Guard,
            Target->id,
            NetworkSimToWorld(Target->position),
            false,
            false,
            TEXT("ONLINE GUARD"),
            EEchoesCommandMarkerType::Guard);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Guard is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned defenders first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target the owned entity to guard with the pointer or center reticle."));
        return;
    }
    FVector2D GuardScreenPosition = FVector2D::ZeroVector;
    const AEchoesEntityView* TargetView =
        ResolveCommandScreenPosition(
            !bKeyboardTargetingEnabled, GuardScreenPosition)
            ? TraceEntityUnderCommandTarget(GuardScreenPosition)
            : nullptr;
    if (TargetView == nullptr)
    {
        TargetView = Cast<AEchoesEntityView>(HitResult.GetActor());
    }
    const echoes::sim::Entity* Target =
        TargetView != nullptr
            ? Bridge->FindEntity(TargetView->GetEntityId())
            : nullptr;
    if (Target == nullptr ||
        Target->owner != UEchoesSimulationSubsystem::LocalPlayerId)
    {
        SetStatusMessage(TEXT("[GUARD_TARGET_INVALID] Point at a live owned entity."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        FString Feedback;
        const TOptional<uint64> SequenceBefore =
            Bridge->GetLastAcceptedLocalCommandSequence();
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::Guard,
                EntityId,
                Target->id,
                Bridge->SimToWorld(Target->position),
                FutureWellChoice,
                Feedback))
        {
            CaptureTutorialAcceptedCommand(
                Bridge,
                SequenceBefore,
                EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("GUARD: %d defender%s assigned to entity %u%s"),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            Target->id,
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            Bridge->SimToWorld(Target->position),
            EEchoesCommandMarkerType::Guard,
            AcceptedCount);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_GUARD_ACCEPTED] source=%s screen=(%.1f,%.1f) target=%u accepted=%d rejected=%d ownerScoped=true"),
            bKeyboardTargetingEnabled ? TEXT("keyboard_reticle") : TEXT("pointer"),
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.X,
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.Y,
            Target->id,
            AcceptedCount,
            RejectedCount);
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[GUARD_REJECTED] No selected entity accepted the guard order.")
                : LastRejection);
    }
}

void AEchoesPlayerController::ShowAcceptedCommandMarker(
    const FVector& WorldLocation,
    EEchoesCommandMarkerType MarkerType,
    int32 AcceptedCount)
{
    UWorld* World = GetWorld();
    if (World == nullptr || AcceptedCount <= 0)
    {
        return;
    }

    if (UEchoesPresentationAudioSubsystem* Audio =
            World->GetSubsystem<UEchoesPresentationAudioSubsystem>())
    {
        Audio->PlayCommandConfirmation();
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.ObjectFlags |= RF_Transient;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesCommandMarkerView* Marker = World->SpawnActor<AEchoesCommandMarkerView>(
        WorldLocation + FVector(0.0f, 0.0f, 8.0f),
        FRotator::ZeroRotator,
        SpawnParameters);
    if (Marker == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_COMMAND_MARKER_FAILED] accepted=%d authorityChanged=false"),
            AcceptedCount);
        return;
    }

    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const bool bReducedMotion =
        Settings != nullptr && Settings->IsReducedMotionEnabled();
    const bool bReducedFlashing =
        Settings != nullptr && Settings->IsReducedFlashingEnabled();
    Marker->InitializeMarker(MarkerType, bReducedMotion, bReducedFlashing);

    const TCHAR* MarkerLabel = TEXT("move");
    switch (MarkerType)
    {
        case EEchoesCommandMarkerType::Attack:
            MarkerLabel = TEXT("attack");
            break;
        case EEchoesCommandMarkerType::AttackMove:
            MarkerLabel = TEXT("attack_move");
            break;
        case EEchoesCommandMarkerType::Patrol:
            MarkerLabel = TEXT("patrol");
            break;
        case EEchoesCommandMarkerType::Guard:
            MarkerLabel = TEXT("guard");
            break;
        case EEchoesCommandMarkerType::Build:
            MarkerLabel = TEXT("build");
            break;
        case EEchoesCommandMarkerType::Interact:
            MarkerLabel = TEXT("interact");
            break;
        case EEchoesCommandMarkerType::Move:
            break;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_COMMAND_MARKER] type=%s accepted=%d formation=%s vfx=selection-command-vfx-v2 authored=true collision=false navigation=false authoritative=false reducedMotion=%s reducedFlashing=%s finalArt=false"),
        MarkerLabel,
        AcceptedCount,
        *GetFormationLabel(),
        bReducedMotion ? TEXT("true") : TEXT("false"),
        bReducedFlashing ? TEXT("true") : TEXT("false"));
}
