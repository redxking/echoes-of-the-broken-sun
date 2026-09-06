#include "EchoesPlayerController.h"
#include "EchoesBuildPlacementPreview.h"
#include "EchoesContextCursorWidget.h"
#include "EchoesEntityView.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesGameUserSettings.h"
#include "EchoesHudLayout.h"
#include "EchoesFieldHudWidget.h"
#include "EchoesCommandMarkerView.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "UnrealClient.h"

#include <algorithm>
#include <optional>

#define LOCTEXT_NAMESPACE "EchoesTacticalInput"

namespace
{
bool IsBuildAction(EEchoesCommandDeckAction Action)
{
    return Action == EEchoesCommandDeckAction::BuildBarracks ||
        Action == EEchoesCommandDeckAction::BuildDropoff ||
        Action == EEchoesCommandDeckAction::BuildUtility;
}
}

void AEchoesPlayerController::InitializeTacticalInputPresentation()
{
    if (!IsLocalController() || ContextCursorWidget != nullptr)
    {
        return;
    }
    ContextCursorWidget = CreateWidget<UEchoesContextCursorWidget>(
        this, UEchoesContextCursorWidget::StaticClass());
    if (ContextCursorWidget != nullptr)
    {
        ContextCursorWidget->SetDesiredSizeInViewport(FVector2D(42.0f, 42.0f));
        ContextCursorWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
        ContextCursorWidget->AddToViewport(10000);
        bShowMouseCursor = false;
    }
}

void AEchoesPlayerController::ShutdownTacticalInputPresentation()
{
    CancelBuildPlacement(false);
    if (ContextCursorWidget != nullptr)
    {
        ContextCursorWidget->RemoveFromParent();
        ContextCursorWidget = nullptr;
    }
    bShowMouseCursor = true;
}

void AEchoesPlayerController::UpdateTacticalInputPresentation()
{
    if (!IsLocalController())
    {
        return;
    }
    if (ContextCursorWidget == nullptr)
    {
        InitializeTacticalInputPresentation();
    }

    UGameViewportClient* ViewportClient =
        GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr;
    const bool bForeground = ViewportClient == nullptr ||
        ViewportClient->Viewport == nullptr ||
        ViewportClient->Viewport->IsForegroundWindow();
    if (!bForeground)
    {
        CancelBuildPlacement(false);
        if (ContextCursorWidget != nullptr)
        {
            ContextCursorWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        bShowMouseCursor = true;
        return;
    }

    // Modal Slate/UMG surfaces own pointer routing and the native UI cursor.
    // Do not let the battlefield cursor hide it again on the next tick.
    if (UsesShellWidget() || IsModalOverlayVisible())
    {
        CancelBuildPlacement(false);
        if (ContextCursorWidget != nullptr)
            ContextCursorWidget->SetVisibility(ESlateVisibility::Collapsed);
        bShowMouseCursor = true;
        DefaultMouseCursor = EMouseCursor::Default;
        return;
    }

    FVector2D Pointer;
    FVector2D ViewportSize;
    if (ContextCursorWidget == nullptr ||
        !ResolvePointerScreenPosition(Pointer, &ViewportSize))
    {
        bShowMouseCursor = true;
        return;
    }
    bShowMouseCursor = false;
    ContextCursorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
    // ResolvePointerScreenPosition returns viewport pixels. Let UMG remove
    // the platform DPI scale exactly once for Retina and scaled HUD modes.
    ContextCursorWidget->SetPositionInViewport(
        Pointer - FVector2D(4.0f, 4.0f), true);

    FEchoesContextCursorFacts Facts;
    Facts.bModal = IsModalOverlayVisible();
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    Facts.bOverMinimap = !Facts.bModal && FieldHudWidget &&
        FieldHudWidget->IsPointerOverMinimap(Pointer);

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const bool bReplay =
        Bridge != nullptr && Bridge->IsReplayPlaybackActive();
    const std::optional<echoes::sim::PlayerView> ScopedView =
        Bridge == nullptr
            ? std::nullopt
            : bReplay
                ? Bridge->GetReplayPresentationPlayerView()
                : Bridge->GetSimulation() != nullptr
                    ? Bridge->GetSimulation()->CreatePlayerView(
                          UEchoesSimulationSubsystem::LocalPlayerId)
                    : std::nullopt;

    if (bReplay)
    {
        CancelBuildPlacement(false);
    }
    else if (bBuildPlacementActive)
    {
        Facts.bBuildPlacement = true;
        FHitResult GroundHit;
        const bool bHasGround = GetHitResultAtScreenPosition(
            Pointer, ECC_Visibility, true, GroundHit);
        FEchoesBuildPlacementEvaluation Evaluation;
        if (bHasGround && ScopedView.has_value())
        {
            BuildPlacementWorldPosition = GroundHit.Location;
            Evaluation = FEchoesBuildPlacementModel::Evaluate(
                *ScopedView,
                BuildPlacementWorkerId,
                BuildPlacementType,
                Bridge->WorldToSim(GroundHit.Location));
        }
        else
        {
            Evaluation.Validity = bHasGround
                ? EEchoesBuildPreviewValidity::InvalidWorker
                : EEchoesBuildPreviewValidity::OutsideMap;
        }
        bBuildPlacementValid = Evaluation.IsValid();
        BuildPlacementHalfExtentRaw = Evaluation.FootprintHalfExtentRaw;
        Facts.bPlacementValid = bBuildPlacementValid;
        if (BuildPlacementPreview != nullptr && bHasGround &&
            BuildPlacementHalfExtentRaw > 0)
        {
            BuildPlacementPreview->SetPreview(
                BuildPlacementWorldPosition,
                BuildPlacementHalfExtentRaw,
                bBuildPlacementValid);
        }
    }
    else if (!Facts.bOverMinimap && ScopedView.has_value())
    {
        AEchoesEntityView* HitView = TraceEntityUnderCommandTarget(Pointer);
        const echoes::sim::Entity* ScopedEntity = nullptr;
        if (HitView != nullptr)
        {
            const uint32 HitId = HitView->GetEntityId();
            const auto Found = std::find_if(
                ScopedView->Entities().begin(),
                ScopedView->Entities().end(),
                [HitId](const echoes::sim::Entity& Entity)
                {
                    return Entity.id == HitId;
                });
            if (Found != ScopedView->Entities().end())
            {
                ScopedEntity = &*Found;
            }
        }
        if (ScopedEntity != nullptr)
        {
            Facts.bFriendlyEntity =
                ScopedEntity->owner == ScopedView->Player().id;
            Facts.bHostileEntity = ScopedView->Config().IsHostile(
                ScopedView->Player().id,
                ScopedEntity->owner);
            Facts.bGatherableEntity =
                ScopedEntity->type == echoes::sim::EntityType::ResourceNode;
        }
        Facts.bTargetActionArmed =
            ArmedDeckAction != EEchoesCommandDeckAction::None &&
            !IsBuildAction(ArmedDeckAction);
        if (Facts.bTargetActionArmed)
        {
            FHitResult GroundHit;
            Facts.bTargetActionValid = GetHitResultAtScreenPosition(
                Pointer, ECC_Visibility, true, GroundHit);
        }
    }

    ContextCursorWidget->SetCursorState(
        FEchoesContextCursorModel::Resolve(Facts),
        Settings != nullptr && Settings->IsHighContrastHudEnabled());
}

void AEchoesPlayerController::BeginBuildPlacement(
    echoes::sim::EntityType BuildingType)
{
    CancelBuildPlacement(false);
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge != nullptr && Bridge->IsReplayPlaybackActive())
    {
        SetStatusMessage(LOCTEXT("ReplayConstructionReadOnly", "REPLAY VIEW — construction is read-only.").ToString());
        return;
    }
    if (IsActiveOnlineNetworkMatch())
    {
        SetStatusMessage(LOCTEXT("OnlinePlacementUnavailable", "[BUILD_PREVIEW_OFFLINE_ONLY] Online construction waits for the authoritative placement path.").ToString());
        return;
    }
    const std::optional<echoes::sim::PlayerView> ScopedView =
        Bridge != nullptr && Bridge->GetSimulation() != nullptr
            ? Bridge->GetSimulation()->CreatePlayerView(
                  UEchoesSimulationSubsystem::LocalPlayerId)
            : std::nullopt;
    if (!ScopedView.has_value())
    {
        SetStatusMessage(LOCTEXT("PlacementNotReady", "[SIM_NOT_READY] Construction preview is unavailable.").ToString());
        return;
    }
    for (const uint32 SelectedId : SelectedEntityIds)
    {
        const auto Worker = std::find_if(
            ScopedView->Entities().begin(),
            ScopedView->Entities().end(),
            [SelectedId](const echoes::sim::Entity& Entity)
            {
                return Entity.id == SelectedId &&
                    Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                    Entity.type == echoes::sim::EntityType::Worker &&
                    Entity.hitPoints > 0;
            });
        if (Worker != ScopedView->Entities().end())
        {
            BuildPlacementWorkerId = SelectedId;
            break;
        }
    }
    if (BuildPlacementWorkerId == 0)
    {
        SetStatusMessage(FEchoesBuildPlacementModel::Feedback(
            EEchoesBuildPreviewValidity::InvalidWorker));
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.ObjectFlags |= RF_Transient;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    BuildPlacementPreview = GetWorld()->SpawnActor<AEchoesBuildPlacementPreview>(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (BuildPlacementPreview == nullptr)
    {
        BuildPlacementWorkerId = 0;
        SetStatusMessage(LOCTEXT("PlacementUnavailable", "[BUILD_PREVIEW_UNAVAILABLE] Construction preview could not be created.").ToString());
        return;
    }
    BuildPlacementPreview->SetActorHiddenInGame(true);
    BuildPlacementType = BuildingType;
    BuildPlacementHalfExtentRaw = 0;
    bBuildPlacementValid = false;
    bBuildPlacementActive = true;
    ArmedDeckAction = EEchoesCommandDeckAction::None;
    SetStatusMessage(LOCTEXT("PlacementInstructions", "Move the blueprint to visible clear ground. Left-click confirms; right-click cancels.").ToString(), 12.0f);
    UpdateTacticalInputPresentation();
}

bool AEchoesPlayerController::ConfirmBuildPlacement()
{
    if (!bBuildPlacementActive)
    {
        return false;
    }
    if (UEchoesSimulationSubsystem* ReplayBridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        ReplayBridge != nullptr && ReplayBridge->IsReplayPlaybackActive())
    {
        CancelBuildPlacement(false);
        SetStatusMessage(LOCTEXT("ReplayConstructionReadOnly", "REPLAY VIEW — construction is read-only.").ToString());
        return false;
    }
    UpdateTacticalInputPresentation();
    if (!bBuildPlacementValid)
    {
        UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        const std::optional<echoes::sim::PlayerView> View =
            Bridge != nullptr && Bridge->GetSimulation() != nullptr
                ? Bridge->GetSimulation()->CreatePlayerView(
                      UEchoesSimulationSubsystem::LocalPlayerId)
                : std::nullopt;
        const FEchoesBuildPlacementEvaluation Evaluation =
            View.has_value() && Bridge != nullptr
                ? FEchoesBuildPlacementModel::Evaluate(
                      *View,
                      BuildPlacementWorkerId,
                      BuildPlacementType,
                      Bridge->WorldToSim(BuildPlacementWorldPosition))
                : FEchoesBuildPlacementEvaluation{};
        SetStatusMessage(FEchoesBuildPlacementModel::Feedback(
            Evaluation.Validity));
        return false;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr || !Bridge->IssueBuildCommand(
            BuildPlacementWorkerId,
            BuildPlacementType,
            BuildPlacementWorldPosition,
            Feedback))
    {
        SetStatusMessage(Feedback.IsEmpty()
            ? LOCTEXT("PlacementRejected", "[BUILD_PLACEMENT_REJECTED] Choose another location.").ToString()
            : Feedback);
        return false;
    }
    ShowAcceptedCommandMarker(
        BuildPlacementWorldPosition,
        EEchoesCommandMarkerType::Build,
        1);
    SetStatusMessage(LOCTEXT("ConstructionQueued", "Construction order queued.").ToString());
    CancelBuildPlacement(false);
    return true;
}

void AEchoesPlayerController::CancelBuildPlacement(bool bShowFeedback)
{
    const bool bWasActive = bBuildPlacementActive;
    if (BuildPlacementPreview != nullptr)
    {
        BuildPlacementPreview->Destroy();
        BuildPlacementPreview = nullptr;
    }
    bBuildPlacementActive = false;
    bBuildPlacementValid = false;
    BuildPlacementWorkerId = 0;
    BuildPlacementHalfExtentRaw = 0;
    if (bWasActive && bShowFeedback)
    {
        SetStatusMessage(LOCTEXT("PlacementCancelled", "Construction placement cancelled.").ToString(), 3.0f);
    }
}

#if WITH_DEV_AUTOMATION_TESTS
// Legacy explicit-position fixture seam. Runtime input belongs to UMG.
bool AEchoesPlayerController::HandleMinimapPointer(
    const FVector2D& ScreenPosition, const FVector2D& ViewportSize, bool bIssueOrder)
{
    if (IsModalOverlayVisible() || ScreenPosition.ContainsNaN() || ViewportSize.X <= 0 || ViewportSize.Y <= 0) return false;
    const auto* Settings = UEchoesGameUserSettings::Get();
    const auto Layout = FEchoesHudLayout::Build(ViewportSize, Settings ? Settings->GetHudScale() : 1.f, false);
    if (!Layout.bMinimapVisible || !Layout.MinimapPanel.IsInsideOrOn(ScreenPosition)) return false;
    const FVector2D Unit = (ScreenPosition - Layout.MinimapPanel.Min) / Layout.MinimapPanel.GetSize();
    const bool bHandled = HandleFieldHudPointer(Unit, bIssueOrder);
    // Retained explicit-position compatibility seam for automation. Native UMG
    // owns minimap mouse capture and does not use this legacy viewport layout.
    if (bHandled && !bIssueOrder) bMinimapDragging = true;
    return bHandled;
}
#endif


bool AEchoesPlayerController::HandleFieldHudPointer(
    const FVector2D& Unit, bool bIssueOrder)
{
    if (Unit.ContainsNaN() || !FMath::IsFinite(Unit.X) || !FMath::IsFinite(Unit.Y) ||
        Unit.X < 0 || Unit.X > 1 || Unit.Y < 0 || Unit.Y > 1 || IsModalOverlayVisible()) return false;
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (IsActiveOnlineNetworkMatch() && !IsReplayInputActive())
    {
        const auto* Keyframe = GetNetworkScopedView();
        if (!Keyframe || Keyframe->mapWidthTiles <= 0 || Keyframe->mapHeightTiles <= 0) return false;
        if (bIssueOrder)
        {
            SetStatusMessage(LOCTEXT("OnlineMinimapOrderUnavailable",
                "Minimap orders are unavailable in online play. Issue the order on the battlefield.").ToString());
            return false;
        }
        const echoes::sim::Vec2 Position{
            echoes::sim::Fixed::FromRaw(FMath::Clamp(FMath::RoundToInt(Unit.X * Keyframe->mapWidthTiles * echoes::sim::kFixedScale), 0, Keyframe->mapWidthTiles * echoes::sim::kFixedScale - 1)),
            echoes::sim::Fixed::FromRaw(FMath::Clamp(FMath::RoundToInt(Unit.Y * Keyframe->mapHeightTiles * echoes::sim::kFixedScale), 0, Keyframe->mapHeightTiles * echoes::sim::kFixedScale - 1))};
        if (auto* Camera = Cast<AEchoesRTSCameraPawn>(GetPawn())) Camera->PanToWorld(NetworkSimToWorld(Position));
        bSelectionButtonDown = false;
        return true;
    }
    if (!Bridge || (!Bridge->IsScenarioReady() && !Bridge->IsReplayPlaybackActive())) return false;
    const auto View = Bridge->IsReplayPlaybackActive()
        ? Bridge->GetReplayPresentationPlayerView()
        : Bridge->GetSimulation()
            ? Bridge->GetSimulation()->CreatePlayerView(
                  UEchoesSimulationSubsystem::LocalPlayerId)
            : std::nullopt;
    const auto* Observer = Bridge->IsReplayPlaybackActive() &&
        Bridge->GetReplayPlaybackState().Perspective == EEchoesReplayPerspective::OmniscientObserver
        ? Bridge->GetReplayPresentationSimulation() : nullptr;
    if (!View && !Observer) return true;
    const auto& Config = View ? View->Config() : Observer->Config();
    const echoes::sim::Vec2 Position{
        echoes::sim::Fixed::FromRaw(FMath::Clamp(FMath::RoundToInt(Unit.X * Config.mapWidthTiles * echoes::sim::kFixedScale), 0, Config.mapWidthTiles * echoes::sim::kFixedScale - 1)),
        echoes::sim::Fixed::FromRaw(FMath::Clamp(FMath::RoundToInt(Unit.Y * Config.mapHeightTiles * echoes::sim::kFixedScale), 0, Config.mapHeightTiles * echoes::sim::kFixedScale - 1))};
    const FVector Destination = Bridge->SimToWorld(Position);
    if (!bIssueOrder)
    {
        if (auto* Camera = Cast<AEchoesRTSCameraPawn>(GetPawn())) Camera->PanToWorld(Destination);
        bSelectionButtonDown = false;
        return true;
    }
    if (Bridge->IsReplayPlaybackActive())
    {
        SetStatusMessage(LOCTEXT("ReplayOrdersReadOnly", "REPLAY VIEW — tactical orders are read-only.").ToString());
        return true;
    }
    // P2's offline adapter consumes only the local scoped view. Hidden markers
    // cannot turn a minimap move into a direct attack on an undiscovered entity.
    PruneSelection();
    if (SelectedEntityIds.IsEmpty()) { SetStatusMessage(LOCTEXT("SelectBeforeOrder", "Select units before issuing an order.").ToString()); return true; }
    uint32 Target = 0;
    int64 Nearest = MAX_int64;
    // Context targeting is in map space so HUD scaling cannot change which
    // visible enemy receives an order. Hidden entities never enter this query.
    const int64 RadiusRaw = echoes::sim::kFixedScale;
    for (const auto& Entity : View->Entities())
    {
        if (!View->Config().IsHostile(UEchoesSimulationSubsystem::LocalPlayerId, Entity.owner)) continue;
        const int64 DX = static_cast<int64>(Entity.position.x.Raw()) - Position.x.Raw();
        const int64 DY = static_cast<int64>(Entity.position.y.Raw()) - Position.y.Raw();
        const int64 Distance = DX*DX + DY*DY;
        if (Distance <= RadiusRaw*RadiusRaw && Distance < Nearest) { Target = Entity.id; Nearest = Distance; }
    }
    const auto Type = Target ? echoes::sim::CommandType::Attack : echoes::sim::CommandType::Move;
    const auto Destinations = BuildSelectedFormationDestinations(Destination, SelectedEntityIds.Num());
    int32 Accepted = 0; FString Feedback;
    for (int32 Index = 0; Index < SelectedEntityIds.Num(); ++Index)
        Accepted += Bridge->IssueCommand(Type, SelectedEntityIds[Index], Target,
            Target ? Destination : Destinations[Index], echoes::sim::FutureWellChoice::Dormant, Feedback) ? 1 : 0;
    if (Accepted > 0)
    {
        ShowAcceptedCommandMarker(Destination, Target ? EEchoesCommandMarkerType::Attack : EEchoesCommandMarkerType::Move, Accepted);
        SetStatusMessage(FText::Format(
            LOCTEXT("MinimapOrderAccepted", "{0} order accepted for {1} {1}|plural(one=unit,other=units)."),
            Target ? LOCTEXT("AttackOrder", "Attack") : LOCTEXT("MoveOrder", "Move"),
            Accepted).ToString());
    }
    else SetStatusMessage(Feedback);
    return true;
}

void AEchoesPlayerController::ToggleTacticalPause()
{
    if (IsModalOverlayVisible() || IsActiveOnlineNetworkMatch()) return;
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (Bridge != nullptr && Bridge->IsReplayPlaybackActive())
    {
        SetStatusMessage(LOCTEXT("ReplayPause", "REPLAY VIEW — use replay transport to pause playback.").ToString());
        return;
    }
    if (!Bridge || !Bridge->IsScenarioReady() || Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing) return;
    if (Bridge->IsScenarioPaused() && !RequireOperationMastery(Bridge->GetOperationMode())) return;
    bTacticalPaused = !Bridge->IsScenarioPaused();
    Bridge->SetScenarioPaused(bTacticalPaused);
    SetStatusMessage(bTacticalPaused ? LOCTEXT("TacticalPaused", "Tactical pause — issue orders, then press Pause to resume.").ToString() : LOCTEXT("BattleResumed", "Battle resumed.").ToString(), bTacticalPaused ? 3600.f : 3.f);
}

#undef LOCTEXT_NAMESPACE
