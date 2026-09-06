// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesPlayerController.h"

#include "EchoesFieldHudWidget.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesGameInstance.h"
#include "EchoesGameUserSettings.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
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
    FieldHudWidget->SetVisibility(View.Surface == EEchoesFieldHudSurface::Hidden
        ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    const bool bModal = !UsesShellWidget() &&
        (View.Technology.bVisible ||
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
        case EEchoesFieldHudAction::OnlineLeave: LeaveOnlineMatch(); break;
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
