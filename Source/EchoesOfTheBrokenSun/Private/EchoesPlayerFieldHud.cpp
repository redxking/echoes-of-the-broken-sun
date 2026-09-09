// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesPlayerController.h"

#include "EchoesFieldHudWidget.h"
#include "EchoesPowerNetworkView.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesGameInstance.h"
#include "EchoesGameUserSettings.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Misc/App.h"
#include "UnrealClient.h"

FEchoesFieldHudView AEchoesPlayerController::BuildFieldHudView() const
{
    FEchoesFieldHudBuildContext Context;
    Context.Controller = this;
    Context.Simulation = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    Context.Settings = UEchoesGameUserSettings::Get();
    Context.Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr;
    int32 Width = 0, Height = 0;
    GetViewportSize(Width, Height);
    if (Width > 0 && Height > 0) Context.ViewportSize = FVector2D(Width, Height);
    Context.RealTimeSeconds = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
    FEchoesFieldHudView View;
    FString Error;
    if (!FEchoesFieldHudModel::Build(Context, View, Error))
    {
        // A missing authority produces no field data. Never substitute a live
        // simulation, or the retired Canvas renderer, for a failed replay view.
        if (LastFieldHudError != Error)
        {
            UE_LOG(LogEchoes, Display, TEXT("[ECHOES_FIELD_HUD_UNAVAILABLE] %s"), *Error);
            LastFieldHudError = Error;
        }
        FEchoesFieldHudView Failure;
        if (Context.Simulation && (Context.Simulation->IsScenarioReady() ||
            Context.Simulation->IsReplayPlaybackActive()))
        {
            Failure.Surface = Context.Simulation->IsReplayPlaybackActive()
                ? EEchoesFieldHudSurface::Replay : EEchoesFieldHudSurface::Battlefield;
            Failure.Status = NSLOCTEXT("EchoesFieldHud", "DisplayUnavailable",
                "Battlefield display unavailable. Open the menu to retry or return to title.");
            if (Context.Settings) Failure.HudScale = Context.Settings->GetHudScale();
        }
        return Failure;
    }
    LastFieldHudError.Reset();
    return View;
}

bool AEchoesPlayerController::IsProductionCancellationCurrent(
    const UEchoesSimulationSubsystem& Bridge) const
{
    if (!PendingProductionCancellation.bVisible ||
        GetNetMode() == NM_Client || IsReplayInputActive() ||
        IsActiveOnlineNetworkMatch() || !Bridge.IsScenarioReady() ||
        Bridge.GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing ||
        Bridge.GetScenarioAuthorityGeneration() !=
            PendingProductionCancellation.AuthorityGeneration ||
        SelectedEntityIds.Num() != 1 ||
        SelectedEntityIds[0] != PendingProductionCancellation.ProducerId)
    {
        return false;
    }

    echoes::sim::ProducerQueueState Queue;
    if (!Bridge.GetLocalProducerQueueState(
            PendingProductionCancellation.ProducerId, Queue))
    {
        return false;
    }

    const echoes::sim::ProductionQueueItem* Item = nullptr;
    int32 Progress = 0;
    if (PendingProductionCancellation.Slot == 0)
    {
        if (!Queue.active)
        {
            return false;
        }
        Item = &Queue.activeItem;
        Progress = Queue.activeProgress;
    }
    else
    {
        const int32 WaitingIndex = PendingProductionCancellation.Slot - 1;
        if (!Queue.waiting.empty() && WaitingIndex >= 0 &&
            WaitingIndex < static_cast<int32>(Queue.waiting.size()))
        {
            Item = &Queue.waiting[static_cast<size_t>(WaitingIndex)];
        }
    }
    if (Item == nullptr || Item->itemId == 0 ||
        Item->itemId != PendingProductionCancellation.ItemId ||
        Item->unitType != PendingProductionCancellation.UnitType ||
        Item->requiredTicks != PendingProductionCancellation.RequiredTicks ||
        Item->configuredCost.material !=
            PendingProductionCancellation.ConfiguredMatter ||
        Item->configuredCost.dawnshards !=
            PendingProductionCancellation.ConfiguredDawn ||
        Item->investedCost.material !=
            PendingProductionCancellation.InvestedMatter ||
        Item->investedCost.dawnshards !=
            PendingProductionCancellation.InvestedDawn ||
        Item->logisticsCost != PendingProductionCancellation.Logistics ||
        Progress != PendingProductionCancellation.Progress)
    {
        return false;
    }
    return true;
}

bool AEchoesPlayerController::GetProductionCancellationConfirmation(
    FEchoesFieldHudProductionCancellationView& OutView) const
{
    OutView = {};
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !IsProductionCancellationCurrent(*Bridge))
    {
        return false;
    }
    OutView.bVisible = true;
    OutView.ProducerId = PendingProductionCancellation.ProducerId;
    OutView.ItemId = PendingProductionCancellation.ItemId;
    OutView.Slot = PendingProductionCancellation.Slot;
    OutView.Unit = PendingProductionCancellation.Unit;
    OutView.ProgressPercent = static_cast<int32>(FMath::Clamp<int64>(
        static_cast<int64>(PendingProductionCancellation.Progress) * 100 /
            FMath::Max(1, PendingProductionCancellation.RequiredTicks),
        0,
        100));
    OutView.RefundPercent = PendingProductionCancellation.RefundPercent;
    OutView.InvestedMatter = PendingProductionCancellation.InvestedMatter;
    OutView.InvestedDawn = PendingProductionCancellation.InvestedDawn;
    OutView.RefundMatter = PendingProductionCancellation.RefundMatter;
    OutView.RefundDawn = PendingProductionCancellation.RefundDawn;
    OutView.bActive = PendingProductionCancellation.Slot == 0;
    return true;
}

bool AEchoesPlayerController::OpenProductionCancellationConfirmation(
    const FEchoesFieldHudProductionView& Production,
    int32 Slot)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || PendingProductionCancellation.bVisible ||
        Slot < 0 || Slot > 4 || GetNetMode() == NM_Client ||
        IsReplayInputActive() || IsActiveOnlineNetworkMatch() ||
        Production.ProducerId == 0 || SelectedEntityIds.Num() != 1 ||
        SelectedEntityIds[0] != Production.ProducerId)
    {
        return false;
    }

    echoes::sim::ProducerQueueState Queue;
    if (!Bridge->GetLocalProducerQueueState(Production.ProducerId, Queue))
    {
        return false;
    }
    const echoes::sim::ProductionQueueItem* Item = nullptr;
    int32 Progress = 0;
    if (Slot == 0)
    {
        if (Queue.active)
        {
            Item = &Queue.activeItem;
            Progress = Queue.activeProgress;
        }
    }
    else
    {
        const int32 WaitingIndex = Slot - 1;
        if (WaitingIndex >= 0 &&
            WaitingIndex < static_cast<int32>(Queue.waiting.size()))
        {
            Item = &Queue.waiting[static_cast<size_t>(WaitingIndex)];
        }
    }
    if (Item == nullptr || Item->itemId == 0 ||
        (Slot > 0 && (Item->investedCost.material != 0 ||
                      Item->investedCost.dawnshards != 0)))
    {
        return false;
    }

    FEchoesPendingProductionCancellation Pending;
    Pending.bVisible = true;
    Pending.bScenarioWasPaused = Bridge->IsScenarioPaused();
    Pending.AuthorityGeneration = Bridge->GetScenarioAuthorityGeneration();
    Pending.ProducerId = Production.ProducerId;
    Pending.ItemId = Item->itemId;
    Pending.Slot = Slot;
    Pending.UnitType = Item->unitType;
    if (const FEchoesFieldHudProductionItem* PresentedItem =
            Production.Items.FindByPredicate(
                [Item](const FEchoesFieldHudProductionItem& Candidate)
                {
                    return Candidate.ItemId == Item->itemId;
                }))
    {
        Pending.Unit = PresentedItem->Unit;
    }
    if (Pending.Unit.IsEmpty())
    {
        return false;
    }
    Pending.Progress = Progress;
    Pending.RequiredTicks = Item->requiredTicks;
    Pending.ConfiguredMatter = Item->configuredCost.material;
    Pending.ConfiguredDawn = Item->configuredCost.dawnshards;
    Pending.InvestedMatter = Item->investedCost.material;
    Pending.InvestedDawn = Item->investedCost.dawnshards;
    Pending.Logistics = Item->logisticsCost;
    Pending.RefundPercent = Slot == 0
        ? (static_cast<int64>(Progress) * 2 < Item->requiredTicks ? 75 : 50)
        : 0;
    Pending.RefundMatter = static_cast<int32>(
        static_cast<int64>(Pending.InvestedMatter) * Pending.RefundPercent /
        100);
    Pending.RefundDawn = static_cast<int32>(
        static_cast<int64>(Pending.InvestedDawn) * Pending.RefundPercent /
        100);
    PendingProductionCancellation = Pending;

    bSelectionButtonDown = false;
    Bridge->SetScenarioPaused(true);
    SetNarrativePlaybackPausedOutsideCinematic(true);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetStatusMessage(
        Slot == 0
            ? FString::Printf(
                  TEXT("REVIEW CANCELLATION — refund %d Matter / %d Dawn."),
                  Pending.RefundMatter,
                  Pending.RefundDawn)
            : TEXT("REVIEW CANCELLATION — this waiting order has not been charged; refund 0 Matter / 0 Dawn."),
        3600.0f);
    return true;
}

void AEchoesPlayerController::CloseProductionCancellationConfirmation(
    bool bRestoreScenarioPause)
{
    if (!PendingProductionCancellation.bVisible)
    {
        return;
    }
    const bool bWasPaused = PendingProductionCancellation.bScenarioWasPaused;
    PendingProductionCancellation = {};
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (bRestoreScenarioPause && Bridge != nullptr &&
        Bridge->IsScenarioReady())
    {
        Bridge->SetScenarioPaused(bWasPaused);
        SetNarrativePlaybackPausedOutsideCinematic(bWasPaused);
    }
    const bool bKeepInputHeld = IsModalOverlayVisible() ||
        (bRestoreScenarioPause && bWasPaused);
    SetIgnoreMoveInput(bKeepInputHeld);
    SetIgnoreLookInput(bKeepInputHeld);
}

void AEchoesPlayerController::ConfirmProductionCancellation()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !IsProductionCancellationCurrent(*Bridge))
    {
        const bool bSameAuthority = Bridge != nullptr &&
            Bridge->IsScenarioReady() &&
            Bridge->GetScenarioAuthorityGeneration() ==
                PendingProductionCancellation.AuthorityGeneration;
        CloseProductionCancellationConfirmation(bSameAuthority);
        SetStatusMessage(TEXT("Production changed. Review the current queue before cancelling."));
        return;
    }

    const uint32 ProducerId = PendingProductionCancellation.ProducerId;
    const uint8 Slot = static_cast<uint8>(PendingProductionCancellation.Slot);
    const uint64 ItemId = PendingProductionCancellation.ItemId;
    FString Feedback;
    const bool bAccepted = Bridge->IssueProductionCancellation(
        ProducerId, Slot, ItemId, Feedback);
    CloseProductionCancellationConfirmation(true);
    if (bAccepted)
    {
        SetStatusMessage(NSLOCTEXT(
            "EchoesFieldHud", "ProductionCancelled", "Production cancellation queued.")
            .ToString());
    }
    else
    {
        SetStatusMessage(NSLOCTEXT(
            "EchoesFieldHud", "ProductionChangeRefused", "Production queue unchanged.")
            .ToString());
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_PRODUCTION_CANCELLATION_REFUSED] producer=%u slot=%u item=%llu reason=%s"),
            ProducerId,
            Slot,
            static_cast<unsigned long long>(ItemId),
            *Feedback);
    }
}

void AEchoesPlayerController::ValidateProductionCancellationConfirmation()
{
    if (!PendingProductionCancellation.bVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge != nullptr && IsProductionCancellationCurrent(*Bridge))
    {
        return;
    }
    const bool bSameAuthority = Bridge != nullptr && Bridge->IsScenarioReady() &&
        Bridge->GetScenarioAuthorityGeneration() ==
            PendingProductionCancellation.AuthorityGeneration;
    CloseProductionCancellationConfirmation(bSameAuthority);
    SetStatusMessage(TEXT("Production changed. Cancellation review closed."));
}

void AEchoesPlayerController::RefreshFieldHud()
{
    if (GetLocalPlayer() == nullptr || !IsLocalController() || FApp::IsUnattended()) return;
    if (!FieldHudWidget)
    {
        FieldHudWidget = CreateWidget<UEchoesFieldHudWidget>(this, UEchoesFieldHudWidget::StaticClass());
        if (!FieldHudWidget) return;
        FieldHudWidget->Configure(this);
        FieldHudWidget->SetView(BuildFieldHudView());
        FieldHudWidget->AddToViewport(20);
    }
    const auto View = BuildFieldHudView();
    FieldHudWidget->SetView(View);
    if (!PowerNetworkView && View.Authority == EEchoesFieldHudAuthority::LivePlayerView &&
        View.Surface == EEchoesFieldHudSurface::Battlefield)
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.ObjectFlags |= RF_Transient;
        PowerNetworkView = GetWorld()->SpawnActor<AEchoesPowerNetworkView>(Params);
    }
    if (PowerNetworkView) PowerNetworkView->SetView(View);
    FieldHudWidget->SetVisibility(View.Surface == EEchoesFieldHudSurface::Hidden
        ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    const uint64 PresentationFrame = GFrameCounter;
    if (TutorialRosterHudCandidateEntity != 0 &&
        PresentationFrame > TutorialRosterHudCandidateFrame)
    {
        ObserveTutorialRosterHudPublication(View, PresentationFrame);
        TutorialRosterHudCandidateEntity = 0;
        TutorialRosterHudCandidateFrame = 0;
    }
    const auto Roster = TutorialSelection.RosterProgress();
    if (TutorialSelection.ActiveStage() ==
            EEchoesTutorialSelectionStage::Roster &&
        Roster.bSingleClickSelected && !Roster.bHudPublished &&
        View.Authority == EEchoesFieldHudAuthority::LivePlayerView &&
        View.Surface == EEchoesFieldHudSurface::Battlefield &&
        View.Selection.bVisible && View.Selection.Entries.Num() == 1 &&
        View.Selection.Entries[0].EntityId == TutorialWorkerId &&
        !View.Selection.Entries[0].Purpose.IsEmpty() &&
        View.Commands.bVisible && !View.Commands.Controls.IsEmpty())
    {
        if (TutorialRosterHudCandidateEntity == 0)
        {
            TutorialRosterHudCandidateEntity = TutorialWorkerId;
            TutorialRosterHudCandidateFrame = PresentationFrame;
        }
    }
    else
    {
        TutorialRosterHudCandidateEntity = 0;
        TutorialRosterHudCandidateFrame = 0;
    }
    const bool bModal = !UsesShellWidget() &&
        (View.Technology.bVisible ||
         View.Production.Cancellation.bVisible ||
         View.Surface == EEchoesFieldHudSurface::CampaignOperations ||
         View.Surface == EEchoesFieldHudSurface::OnlineFrontDoor ||
         View.Surface == EEchoesFieldHudSurface::NetworkLobby ||
         View.Surface == EEchoesFieldHudSurface::OnlineLocalMenu ||
         View.Surface == EEchoesFieldHudSurface::Reconnect);
    if (bModal)
    {
        if (!bFieldHudWasModal || LastFieldHudSurface != View.Surface)
        {
            // UI navigation takes priority; unhandled keys retain the existing
            // controller back/cancel routes, which are gated by modal state.
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            Mode.SetWidgetToFocus(FieldHudWidget->TakeWidget());
            SetInputMode(Mode);
            bSelectionButtonDown = false;
            DefaultMouseCursor = EMouseCursor::Default;
            FieldHudWidget->SetUserFocus(this);
            FieldHudWidget->FocusDefaultAction();
        }
        else if (!FieldHudWidget->HasUserFocus(this) &&
                 !FieldHudWidget->HasUserFocusedDescendants(this))
        {
            const UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
            if (Viewport && Viewport->Viewport && Viewport->Viewport->IsForegroundWindow())
            {
                FieldHudWidget->SetUserFocus(this);
                FieldHudWidget->FocusDefaultAction();
            }
        }
    }
    else if (bFieldHudWasModal && !UsesShellWidget())
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(Mode);
        DefaultMouseCursor = EMouseCursor::Crosshairs;
        UWidgetBlueprintLibrary::SetFocusToGameViewport();
    }
    bFieldHudWasModal = bModal;
    LastFieldHudSurface = View.Surface;
}

void AEchoesPlayerController::HandleFieldHudAction(EEchoesFieldHudAction Action, int32 Argument)
{
    ValidateProductionCancellationConfirmation();
    const auto View = BuildFieldHudView();
    if (View.Surface == EEchoesFieldHudSurface::Hidden || View.Surface == EEchoesFieldHudSurface::Replay) return;
    const auto Contains = [Action, Argument](const TArray<FEchoesFieldHudControl>& Controls)
    {
        return Controls.ContainsByPredicate([Action, Argument](const auto& Control)
        {
            return Control.bEnabled && Control.Action == Action && Control.Argument == Argument;
        });
    };
    bool bAllowed = (View.Surface == EEchoesFieldHudSurface::Battlefield &&
        !IsModalOverlayVisible() && View.Commands.bVisible && Contains(View.Commands.Controls)) ||
        (View.Surface == EEchoesFieldHudSurface::Battlefield &&
            !IsModalOverlayVisible() && View.Production.bVisible &&
            Contains(View.Production.Controls)) ||
        (View.Surface == EEchoesFieldHudSurface::Battlefield &&
            View.Production.Cancellation.bVisible &&
            Contains(View.Production.Controls) &&
            (Action == EEchoesFieldHudAction::ProductionCancelConfirm ||
             Action == EEchoesFieldHudAction::ProductionCancelBack)) ||
        (View.Surface == EEchoesFieldHudSurface::Battlefield && !IsModalOverlayVisible() &&
            View.bObjectiveVisible && Contains(View.ObjectiveControls)) ||
        (View.Surface == EEchoesFieldHudSurface::Battlefield &&
            !IsModalOverlayVisible() && View.Menu.bVisible &&
            View.Menu.Control.bEnabled &&
            Action == View.Menu.Control.Action &&
            Argument == View.Menu.Control.Argument) ||
        (View.Surface == EEchoesFieldHudSurface::Battlefield &&
            !IsModalOverlayVisible() && View.Resources.bVisible &&
            View.Resources.MonitorControl.bEnabled &&
            Action == View.Resources.MonitorControl.Action &&
            Argument == View.Resources.MonitorControl.Argument) ||
        (View.TutorialSkipModal.bVisible && Contains(View.TutorialSkipModal.Controls)) ||
        (Action == EEchoesFieldHudAction::OpenTutorialSkipModal && bTutorialOperationAuthorized) ||
        (View.Campaign.bVisible && Contains(View.Campaign.Controls)) ||
        (View.Online.bVisible && Contains(View.Online.Controls));
    if (View.Technology.bVisible)
    {
        if (Action == EEchoesFieldHudAction::ToggleTechnology ||
            Action == EEchoesFieldHudAction::TechnologyPrevious ||
            Action == EEchoesFieldHudAction::TechnologyNext) bAllowed = true;
        if (Action == EEchoesFieldHudAction::TechnologyResearchTier &&
            View.Technology.Tiers.IsValidIndex(Argument))
            bAllowed = View.Technology.Tiers[Argument].bEnabled;
    }
    if (View.Campaign.bVisible && Action == EEchoesFieldHudAction::CampaignSelectNode)
        bAllowed = View.Campaign.Layout.Nodes.IsValidIndex(Argument);
    if (!bAllowed) return;

    auto* Instance = GetEchoesGameInstance();
    switch (Action)
    {
        case EEchoesFieldHudAction::CommandDeck:
            ActivateCommandDeckAction(static_cast<EEchoesCommandDeckAction>(Argument)); break;
        case EEchoesFieldHudAction::ActivateRelaySupply: ActivateRelaySupply(); break;
        case EEchoesFieldHudAction::ToggleTechnology: ToggleTechnologyPanel(); break;
        case EEchoesFieldHudAction::TechnologyPrevious: FocusPreviousTechnologyTier(); break;
        case EEchoesFieldHudAction::TechnologyNext: FocusNextTechnologyTier(); break;
        case EEchoesFieldHudAction::TechnologyResearchTier: ResearchTechnologyByTier(Argument); break;
        case EEchoesFieldHudAction::CampaignSelectNode: SetSelectedCampaignMapNodeIndex(Argument); break;
        case EEchoesFieldHudAction::CampaignDeploy: DeploySelectedCampaignOperation(); break;
        case EEchoesFieldHudAction::CampaignBack: CloseCampaignOperationsMap(); break;
        case EEchoesFieldHudAction::OnlineHost:
            if (Instance) { Instance->FocusOnlineAction(0); Instance->RequestFixedRulesHost(GetWorld()); } break;
        case EEchoesFieldHudAction::OnlineEditEndpoint:
            if (Instance) Instance->FocusOnlineAction(1); break;
        case EEchoesFieldHudAction::OnlineJoin:
            if (Instance) { Instance->FocusOnlineAction(2); Instance->RequestDirectJoin(this); } break;
        case EEchoesFieldHudAction::OnlineCopyHostAddress: CopyOnlineHostEndpoint(); break;
        case EEchoesFieldHudAction::OnlineBack: CancelOnlineFrontDoor(); break;
        case EEchoesFieldHudAction::OnlineRetry:
            if (Instance) Instance->RetryOnlineFrontDoor(this); break;
        case EEchoesFieldHudAction::NetworkReady: ConfirmPrimaryAction(); break;
        case EEchoesFieldHudAction::OnlineResume: TogglePauseMenu(); break;
        case EEchoesFieldHudAction::OpenPauseMenu: TogglePauseMenu(); break;
        case EEchoesFieldHudAction::OpenResourceMonitor: OpenResourceMonitor(); break;
        case EEchoesFieldHudAction::OnlineOptions:
            OpenOnlineLocalMenuShellScreen(EEchoesShellScreen::Options);
            break;
        case EEchoesFieldHudAction::OnlineControls:
            OpenOnlineLocalMenuShellScreen(EEchoesShellScreen::Controls);
            break;
        case EEchoesFieldHudAction::OnlineCommandHistory:
            OpenOnlineLocalMenuShellScreen(EEchoesShellScreen::FeedbackHistory);
            break;
        case EEchoesFieldHudAction::OnlineLeave: LeaveOnlineMatch(); break;
        case EEchoesFieldHudAction::ProductionCancelConfirm:
            ConfirmProductionCancellation();
            break;
        case EEchoesFieldHudAction::ProductionCancelBack:
            CloseProductionCancellationConfirmation(true);
            SetStatusMessage(TEXT("Production cancellation closed."));
            break;
        case EEchoesFieldHudAction::ProductionCancel:
        case EEchoesFieldHudAction::ProductionMoveUp:
        case EEchoesFieldHudAction::ProductionMoveDown:
        {
            UEchoesSimulationSubsystem* Bridge = GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
            if (Bridge == nullptr || View.Production.ProducerId == 0 ||
                Argument < 0 || Argument > 4)
            {
                return;
            }
            FString Feedback;
            bool bAccepted = false;
            if (Action == EEchoesFieldHudAction::ProductionCancel)
            {
                bAccepted = OpenProductionCancellationConfirmation(
                    View.Production, Argument);
            }
            else
            {
                const int32 ToSlot =
                    Action == EEchoesFieldHudAction::ProductionMoveUp
                        ? Argument - 1
                        : Argument + 1;
                if (ToSlot <= 0 || ToSlot > 4)
                {
                    return;
                }
                bAccepted = Bridge->IssueProductionReorder(
                    View.Production.ProducerId,
                    static_cast<uint8>(Argument),
                    static_cast<uint8>(ToSlot),
                    Feedback);
            }
            if (bAccepted)
            {
                if (Action != EEchoesFieldHudAction::ProductionCancel)
                {
                    SetStatusMessage(NSLOCTEXT(
                              "EchoesFieldHud", "ProductionReordered", "Waiting order moved.")
                              .ToString());
                }
            }
            else
            {
                SetStatusMessage(NSLOCTEXT(
                    "EchoesFieldHud", "ProductionChangeRefused", "Production queue unchanged.")
                    .ToString());
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_PRODUCTION_QUEUE_ACTION_REFUSED] producer=%u slot=%d action=%d reason=%s"),
                    View.Production.ProducerId,
                    Argument,
                    static_cast<int32>(Action),
                    *Feedback);
            }
            break;
        }
        case EEchoesFieldHudAction::AcknowledgeTutorialRejection:
            ObserveTutorialRejectionAcknowledged(TutorialPendingRejectionAttempt); break;
        case EEchoesFieldHudAction::InspectTutorialReserve:
        {
            // This action expands the real current reserve observation in the
            // readiness panel; it never supplies the economic success predicate.
            bTutorialReserveMonitorInspected = true;
            SetStatusMessage(FString::Printf(TEXT("RESERVE: %llu Matter credited by the staged Surveyor. Cargo is booked only on delivery to an operational drop-off; the route continues after unloading."),
                static_cast<unsigned long long>(TutorialOrders.DeliveredMatterObserved())), 15.0f);
            break;
        }
        case EEchoesFieldHudAction::OpenTutorialSkipModal:
            OpenTutorialSkipModal();
            break;
        case EEchoesFieldHudAction::TutorialSkipCurrentStep:
            SkipTutorialCurrentStep();
            break;
        case EEchoesFieldHudAction::TutorialEndAll:
            EndAllTutorials();
            break;
        case EEchoesFieldHudAction::TutorialCancelSkipModal:
            CancelTutorialSkipModal();
            break;
        case EEchoesFieldHudAction::None: return;
    }
    RefreshShell();
    RefreshFieldHud();
}

void AEchoesPlayerController::HandleFieldHudEndpoint(const FString& Endpoint)
{
    const auto View = BuildFieldHudView();
    if (!View.Online.bVisible || !View.Online.Controls.ContainsByPredicate([](const auto& Control)
        { return Control.bEnabled && Control.Action == EEchoesFieldHudAction::OnlineEditEndpoint; })) return;
    if (auto* Instance = GetEchoesGameInstance()) Instance->SetDirectConnectEndpoint(Endpoint);
    RefreshFieldHud();
}
