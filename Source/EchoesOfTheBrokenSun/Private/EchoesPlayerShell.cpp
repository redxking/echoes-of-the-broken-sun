#include "EchoesPlayerController.h"
#include "EchoesShellWidget.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesInterfaceAudioSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "HAL/PlatformTime.h"
#include "GenericPlatform/GenericApplication.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "EchoesPlayerShell"

bool AEchoesPlayerController::RequireOperationMastery(EEchoesOperationMode Operation, bool bLearningCheckpoint)
{
    // A non-player controller used by runtime fixtures has no profile authority.
    // Every local player, and any controller exercising the profile flow, does.
    if (GetLocalPlayer() == nullptr && !bPlayerProfileInitialized) return true;
    if (!bPlayerProfileInitialized) InitializePlayerProfile();
    if (bPlayerProfileAvailable && (PlayerProfile.IsTutorialMasteryComplete() ||
            (Operation == EEchoesOperationMode::CampaignPrologue &&
                (bTutorialOperationAuthorized || bLearningCheckpoint))))
    {
        return true;
    }
    ShellMessage = bPlayerProfileAvailable
        ? LOCTEXT("TrainingRequired", "Complete the tutorial before deploying into campaign or skirmish. Opting out leaves training available from the main menu.").ToString()
        : LOCTEXT("ProfileRequiredForDeployment", "Recover your player profile before deploying.").ToString();
    if (auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr)
        Bridge->SetScenarioPaused(true);
    SetStatusMessage(ShellMessage, 3600.f);
    return false;
}

bool AEchoesPlayerController::InitializePlayerProfile()
{
    if (bPlayerProfileInitialized) return bPlayerProfileAvailable;
    bool bExists = false;
    FEchoesPlayerProfile Candidate;
    FString Feedback;
    bPlayerProfileInitialized = true;
    bPlayerProfileAvailable = FEchoesPlayerProfileStore::LoadWithBackup(
        FEchoesPlayerProfileStore::GetDefaultPath(), Candidate, bExists, Feedback);
    if (!bPlayerProfileAvailable)
    {
        ShellMessage = Feedback;
        PlayerFlow.Push(EEchoesShellScreen::Error);
        return false;
    }
    if (UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
    {
        if (bExists)
        {
            if (!Candidate.ApplySettings(*Settings, Feedback))
            {
                bPlayerProfileAvailable = false;
                ShellMessage = Feedback;
                PlayerFlow.Push(EEchoesShellScreen::Error);
                return false;
            }
        }
        else Candidate.CaptureSettings(*Settings);
        if (GetLocalPlayer() && !FApp::IsUnattended())
        {
            if (GetWorld()->WorldType == EWorldType::PIE) Settings->ApplyNonResolutionSettings();
            else Settings->ApplySettings(false);
        }
    }
    PlayerProfile = Candidate;
    if (UEchoesSimulationSubsystem* Bridge = GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>())
    {
        if (!Bridge->SelectJourneySlot(PlayerProfile.ActiveJourneySlot, Feedback))
        {
            // A damaged journey cannot invalidate the independent profile or
            // prevent choosing another slot. Retry retains the requested slot.
            PendingShellAction = EEchoesShellAction::SelectSlot;
            PendingShellArgument = PlayerProfile.ActiveJourneySlot;
            PlayerProfile.ActiveJourneySlot = static_cast<uint8>(Bridge->GetActiveJourneySlot());
            ShellMessage = Feedback;
            PlayerFlow.Push(EEchoesShellScreen::Error);
            return false;
        }
    }
    if (bExists && !Feedback.IsEmpty()) ShellMessage = Feedback;
    return true;
}

bool AEchoesPlayerController::CommitPlayerProfile()
{
    if (!bPlayerProfileAvailable)
    {
        ShellMessage = LOCTEXT("ProfileRecoveryRequired", "Your profile could not be loaded. Recover it before saving changes.").ToString();
        return false;
    }
    FEchoesPlayerProfile Candidate = PlayerProfile;
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings) Candidate.CaptureSettings(*Settings);
    FString Feedback;
    if (!FEchoesPlayerProfileStore::SaveAtomic(FEchoesPlayerProfileStore::GetDefaultPath(), Candidate, Feedback))
    {
        ShellMessage = Feedback;
        return false;
    }
    PlayerProfile = Candidate;
    if (Settings) Settings->SaveSettings();
    return true;
}

bool AEchoesPlayerController::UsesShellWidget() const
{
    return ShellWidget != nullptr && ShellWidget->IsVisible() &&
        PlayerFlow.Current() != EEchoesShellScreen::Gameplay && PlayerFlow.Current() != EEchoesShellScreen::ReplayTransport &&
        !IsOnlineFrontDoorVisible() && !bCampaignOperationsMapVisible && !bOnlineLocalMenuVisible;
}

void AEchoesPlayerController::RefreshShell()
{
    if (DisplayRevertDeadline > 0.0 && FPlatformTime::Seconds() >= DisplayRevertDeadline) RevertPendingDisplay();
    if (GetLocalPlayer() == nullptr || !IsLocalController() || FApp::IsUnattended()) return;
    if (!bPlayerProfileInitialized) InitializePlayerProfile();
    if (ShellWidget == nullptr)
    {
        ShellWidget = CreateWidget<UEchoesShellWidget>(this, UEchoesShellWidget::StaticClass());
        if (ShellWidget == nullptr) return;
        // Slate caches the root at attachment; construct it before AddToViewport.
        ShellWidget->SetView(BuildShellView());
        ShellWidget->AddToViewport(100);
    }
    const bool bShow = PlayerFlow.Current() != EEchoesShellScreen::Gameplay &&
        !IsOnlineFrontDoorVisible() && !bCampaignOperationsMapVisible && !bOnlineLocalMenuVisible;
    ShellWidget->SetVisibility(!bShow ? ESlateVisibility::Collapsed :
        PlayerFlow.Current() == EEchoesShellScreen::ReplayTransport ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Visible);
    if (bShow) ShellWidget->SetView(BuildShellView());
    const bool bModal = bShow && PlayerFlow.Current() != EEchoesShellScreen::ReplayTransport;
    if (bModal)
    {
        ShellWidget->SetView(BuildShellView());
        if (!bShellWasVisible)
        {
            // Release any battlefield capture when a modal takes ownership.
            // Focusing a child alone leaves the viewport's mouse capture active.
            FInputModeUIOnly Mode;
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            Mode.SetWidgetToFocus(ShellWidget->TakeWidget());
            SetInputMode(Mode);
            DefaultMouseCursor = EMouseCursor::Default;
            ShellWidget->SetKeyboardFocus();
        }
        else if (!ShellWidget->HasUserFocus(this) && !ShellWidget->HasUserFocusedDescendants(this))
        {
            const UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
            if (ViewportClient && ViewportClient->Viewport && ViewportClient->Viewport->IsForegroundWindow())
                ShellWidget->SetUserFocus(this);
        }
    }
    else if (bShellWasVisible)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(Mode);
        DefaultMouseCursor = EMouseCursor::Crosshairs;
        UWidgetBlueprintLibrary::SetFocusToGameViewport();
    }
    bShellWasVisible = bModal;
}

FEchoesShellView AEchoesPlayerController::BuildShellView() const
{
    FEchoesShellView View;
    View.Screen = PlayerFlow.Current();
    View.Eyebrow = LOCTEXT("World", "SORYN / ECHOES OF THE BROKEN SUN");
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings) { View.Scale = Settings->GetHudScale(); View.bHighContrast = Settings->IsHighContrastHudEnabled(); }
    const UEchoesSimulationSubsystem* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    View.Status = FText::FromString(ShellMessage);
    const auto Button = [&View](FText Label, EEchoesShellAction Action, bool bEnabled = true, int32 Argument = 0)
    { View.Buttons.Add({Label, Action, Argument, bEnabled}); };
    const auto Back = [&]() { Button(LOCTEXT("Back", "Back"), EEchoesShellAction::Back); };
    switch (View.Screen)
    {
    case EEchoesShellScreen::Title:
        View.Title = LOCTEXT("Title", "Echoes of the Broken Sun");
        View.Body = LOCTEXT("TitleBody", "The sun is broken. The future is still yours to choose.");
        if (!PlayerProfile.IsTutorialMasteryComplete())
        {
            Button(LOCTEXT("StartTutorial", "Start tutorial"), EEchoesShellAction::Tutorial);
            Button(LOCTEXT("Campaign", "Campaign"), EEchoesShellAction::Campaign);
        }
        else
        {
            Button(LOCTEXT("Campaign", "Campaign"), EEchoesShellAction::Campaign);
            Button(LOCTEXT("Tutorial", "Tutorial"), EEchoesShellAction::Tutorial);
        }
        Button(LOCTEXT("Skirmish", "Skirmish"), EEchoesShellAction::Modes);
        Button(LOCTEXT("ReplayArchive", "Replays"), EEchoesShellAction::OpenReplayBrowser);
        Button(LOCTEXT("Options", "Options"), EEchoesShellAction::Options);
        Button(LOCTEXT("Journeys", "Journeys and recovery"), EEchoesShellAction::SaveLoad);
        Button(LOCTEXT("Credits", "Credits"), EEchoesShellAction::Credits);
        Button(LOCTEXT("Quit", "Quit"), EEchoesShellAction::Quit);
        break;
    case EEchoesShellScreen::Modes:
    {
        View.Title = LOCTEXT("SkirmishSetup", "Skirmish setup");
        const auto& Setup = PendingSkirmishSetup;
        const FText Labels[] = {LOCTEXT("LocalFaction", "Your faction"), LOCTEXT("OpponentFaction", "Opponent faction"),
            LOCTEXT("Teams", "Teams"), LOCTEXT("Map", "Map"), LOCTEXT("AI", "Opponent strategy"),
            LOCTEXT("Difficulty", "Difficulty"), LOCTEXT("Resources", "Resources"), LOCTEXT("VictoryRule", "Victory"), LOCTEXT("Speed", "Speed")};
        const TCHAR* Values[] = {FEchoesSkirmishSetupModel::FactionDisplayName(Setup.LocalFaction),
            FEchoesSkirmishSetupModel::FactionDisplayName(Setup.OpponentFaction), FEchoesSkirmishSetupModel::TeamSetupDisplayName(Setup.TeamSetup),
            FEchoesSkirmishSetupModel::MapDisplayName(Setup.MapPreset), FEchoesSkirmishSetupModel::AiDisplayName(Setup.AiPersonality),
            FEchoesSkirmishSetupModel::DifficultyDisplayName(Setup.Difficulty), FEchoesSkirmishSetupModel::ResourceDisplayName(Setup.ResourceLevel),
            FEchoesSkirmishSetupModel::VictoryConditionDisplayName(Setup.VictoryCondition), FEchoesSkirmishSetupModel::GameSpeedDisplayName(Setup.GameSpeed)};
        TArray<FText> Rows;
        for (int32 Row = 0; Row < 9; ++Row) Rows.Add(FText::Format(LOCTEXT("SetupRow", "{0}{1}: {2}"),
            Row == SkirmishSetupFocusRow ? LOCTEXT("SelectedSetting", "Selected — ") : FText::GetEmpty(), Labels[Row], FText::FromString(Values[Row])));
        View.Body = FText::Join(FText::FromString(TEXT("\n")), Rows);
        Button(LOCTEXT("PreviousSetting", "Previous setting"), EEchoesShellAction::PreviousSetting);
        Button(LOCTEXT("NextSetting", "Next setting"), EEchoesShellAction::NextSetting);
        Button(LOCTEXT("Decrease", "Previous value"), EEchoesShellAction::DecreaseSetting);
        Button(LOCTEXT("Increase", "Next value"), EEchoesShellAction::IncreaseSetting);
        Button(LOCTEXT("ReviewDeployment", "Review deployment"), EEchoesShellAction::Primary);
        Back(); break;
    }
    case EEchoesShellScreen::Briefing:
    {
        View.Title = Bridge ? FText::FromString(Bridge->GetOperationLabel()) : LOCTEXT("Brief", "Mission briefing");
        View.Body = FText::FromString(GetStatusMessage());
        const bool bUnlocked = bPlayerProfileAvailable && Bridge &&
            ((Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue && bTutorialOperationAuthorized) || PlayerProfile.IsTutorialMasteryComplete());
        Button(LOCTEXT("Deploy", "Deploy"), EEchoesShellAction::Primary, Bridge && Bridge->IsScenarioReady() && bUnlocked);
        if (!bUnlocked && bPlayerProfileAvailable)
        {
            View.Status = LOCTEXT("TrainingDeployLocked", "Deployment unlocks after tutorial mastery. Your setup choices are retained.");
            Button(LOCTEXT("StartTutorial", "Start tutorial"), EEchoesShellAction::Tutorial);
        }
        Back(); break;
    }
    case EEchoesShellScreen::Pause:
        View.Title = LOCTEXT("Paused", "Paused");
        View.Body = LOCTEXT("PauseBody", "The battlefield is held. Resume when you are ready.");
        Button(LOCTEXT("Resume", "Resume"), EEchoesShellAction::Resume);
        Button(LOCTEXT("Options", "Options"), EEchoesShellAction::Options);
        Button(LOCTEXT("SaveLoad", "Save and load"), EEchoesShellAction::SaveLoad);
        Button(LOCTEXT("Restart", "Restart mission"), EEchoesShellAction::Restart);
        if (Bridge && Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish)
            Button(LOCTEXT("Concede", "Concede match"), EEchoesShellAction::Concede);
        Button(LOCTEXT("ExitMenu", "Exit to menu"), EEchoesShellAction::ReturnToMenu);
        break;
    case EEchoesShellScreen::Results:
        View.Title = DidPresentedLocalPlayerWin() ? LOCTEXT("Victory", "Victory") : LOCTEXT("Result", "Operation concluded");
        AppendMatchResultDossier(View);
        if (bCampaignResult && CanAdvanceCampaignResult()) Button(LOCTEXT("Continue", "Continue"), EEchoesShellAction::Primary);
        Button(LOCTEXT("Restart", "Restart mission"), EEchoesShellAction::Restart);
        Button(LOCTEXT("ViewReplay", "View replay"), EEchoesShellAction::ViewReplay,
            Bridge && Bridge->GetReplayArchiveState() == EEchoesReplayArchiveState::Succeeded);
        if (Bridge && Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish && !IsActiveOnlineNetworkMatch())
            Button(LOCTEXT("Rematch", "Rematch"), EEchoesShellAction::Rematch);
        Button(LOCTEXT("Menu", "Return to menu"), EEchoesShellAction::ReturnToMenu);
        break;
    case EEchoesShellScreen::ReplayBrowser:
    case EEchoesShellScreen::ReplayTransport:
        BuildReplayShellView(View);
        break;
    case EEchoesShellScreen::SaveLoad:
        View.Title = LOCTEXT("SaveTitle", "Journeys and recovery");
        View.Body = FText::Format(LOCTEXT("SlotBody", "Active journey: Slot {0}\nEach journey keeps its own decisions, endings and checkpoints."), FText::AsNumber(Bridge ? Bridge->GetActiveJourneySlot() : 1));
        for (int32 Slot = 1; Slot <= 3; ++Slot)
            Button(FText::Format(LOCTEXT("Slot", "Select Slot {0}"), FText::AsNumber(Slot)), EEchoesShellAction::SelectSlot,
                PlayerFlow.BaseScreen() == EEchoesShellScreen::Title, Slot);
        if (PlayerFlow.BaseScreen() == EEchoesShellScreen::Pause)
        {
            Button(LOCTEXT("SaveCheckpoint", "Save checkpoint"), EEchoesShellAction::Save, Bridge && !Bridge->IsCheckpointSavePending());
            Button(LOCTEXT("LoadCheckpoint", "Load checkpoint"), EEchoesShellAction::Load);
        }
        else
        {
            Button(LOCTEXT("Recover", "Recover interrupted session"), EEchoesShellAction::Recover);
            Button(LOCTEXT("NewJourney", "Start a new journey in this slot"), EEchoesShellAction::NewJourney);
            Button(LOCTEXT("RestoreJourney", "Restore previous journey"), EEchoesShellAction::RestoreJourney, Bridge && Bridge->HasRestorableCampaignBackup());
        }
        if (Bridge && Bridge->GetCheckpointSaveStatus().State != EEchoesCheckpointSaveState::Idle)
            View.Status = FText::FromString(Bridge->GetCheckpointSaveStatus().Feedback);
        Back(); break;
    case EEchoesShellScreen::Confirmation:
        View.Title = LOCTEXT("ConfirmTitle", "Confirm your choice");
        View.Body = FText::FromString(ShellMessage);
        View.Status = FText::GetEmpty();
        Button(LOCTEXT("Cancel", "Cancel"), EEchoesShellAction::Cancel);
        Button(LOCTEXT("Confirm", "Confirm"), EEchoesShellAction::Confirm);
        break;
    case EEchoesShellScreen::Error:
        View.Title = LOCTEXT("ErrorTitle", "Unable to complete that action");
        View.Body = FText::FromString(ShellMessage);
        View.Status = FText::GetEmpty();
        Button(LOCTEXT("Retry", "Retry"), EEchoesShellAction::Retry);
        if (!bPlayerProfileAvailable) Button(LOCTEXT("ResetProfile", "Create a new local profile"), EEchoesShellAction::ResetProfile);
        else if (!PlayerProfile.IsTutorialMasteryComplete())
            Button(LOCTEXT("StartTutorial", "Start tutorial"), EEchoesShellAction::Tutorial);
        Back(); break;
    case EEchoesShellScreen::Credits:
        View.Title = LOCTEXT("Credits", "Credits");
        View.Body = LOCTEXT("Creator", "Echoes of the Broken Sun\nCreated by Angelis Pseftis");
        Back(); break;
    case EEchoesShellScreen::DisplayConfirmation:
        View.Title = LOCTEXT("KeepDisplayTitle", "Keep these display settings?");
        View.Body = LOCTEXT("KeepDisplayBody", "Choose Keep to save this display mode. It will revert automatically after 15 seconds.");
        Button(LOCTEXT("RevertDisplay", "Revert"), EEchoesShellAction::RevertDisplay);
        Button(LOCTEXT("KeepDisplay", "Keep"), EEchoesShellAction::KeepDisplay);
        break;
    case EEchoesShellScreen::Options:
        View.Title = LOCTEXT("Options", "Options");
        View.Body = LOCTEXT("OptionsBody", "Audio, camera and UI changes are saved immediately. Apply a display change, then choose Keep.");
        if (Settings)
        {
            View.Sliders.Add({FText::Format(LOCTEXT("ScaleSlider", "UI scale: {0}%"), FText::AsNumber(FMath::RoundToInt(Settings->GetHudScale()*100))), EEchoesShellAction::HudScaleValue, Settings->GetHudScale(), .8f, 1.5f});
            Button(FText::Format(LOCTEXT("ScaleDown", "UI scale: {0}% — decrease"), FText::AsNumber(FMath::RoundToInt(Settings->GetHudScale()*100))), EEchoesShellAction::HudScaleDown, Settings->GetHudScale() > .8f);
            Button(LOCTEXT("ScaleUp", "Increase UI scale"), EEchoesShellAction::HudScaleUp, Settings->GetHudScale() < 1.5f);
            const auto Toggle = [&](FText Label, bool bOn, EEchoesShellAction Action)
            { Button(FText::Format(LOCTEXT("Toggle", "{0}: {1}"), Label, bOn ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off")), Action); };
            Toggle(LOCTEXT("Contrast", "High contrast"), Settings->IsHighContrastHudEnabled(), EEchoesShellAction::HighContrast);
            Toggle(LOCTEXT("Motion", "Reduced motion"), Settings->IsReducedMotionEnabled(), EEchoesShellAction::ReducedMotion);
            Toggle(LOCTEXT("Flashing", "Reduced flashing"), Settings->IsReducedFlashingEnabled(), EEchoesShellAction::ReducedFlashing);
            Toggle(LOCTEXT("EdgePan", "Edge pan"), Settings->IsEdgePanEnabled(), EEchoesShellAction::EdgePan);
            Toggle(LOCTEXT("DynamicRange", "Reduced dynamic range"), Settings->IsReducedDynamicRangeEnabled(), EEchoesShellAction::DynamicRange);
            Button(FText::Format(LOCTEXT("PanDown", "Camera pan speed: {0}% — decrease"), FText::AsNumber(FMath::RoundToInt(Settings->GetCameraPanSpeedScale()*100))), EEchoesShellAction::CameraPanDown, Settings->GetCameraPanSpeedScale() > .5f);
            Button(LOCTEXT("PanUp", "Increase camera pan speed"), EEchoesShellAction::CameraPanUp, Settings->GetCameraPanSpeedScale() < 2.f);
            Button(FText::Format(LOCTEXT("ZoomDown", "Camera zoom step: {0}% — decrease"), FText::AsNumber(FMath::RoundToInt(Settings->GetCameraZoomScale()*100))), EEchoesShellAction::CameraZoomDown, Settings->GetCameraZoomScale() > .5f);
            Button(LOCTEXT("ZoomUp", "Increase camera zoom step"), EEchoesShellAction::CameraZoomUp, Settings->GetCameraZoomScale() < 2.f);
            Button(FText::Format(LOCTEXT("ResolutionDown", "Resolution: {0} × {1} — previous"), FText::AsNumber(PendingDisplayResolution.X, &FNumberFormattingOptions::DefaultNoGrouping()), FText::AsNumber(PendingDisplayResolution.Y, &FNumberFormattingOptions::DefaultNoGrouping())), EEchoesShellAction::ResolutionPrevious);
            Button(LOCTEXT("ResolutionUp", "Next resolution"), EEchoesShellAction::ResolutionNext);
            const FText ModeName = PendingDisplayMode == EWindowMode::Windowed ? LOCTEXT("Windowed", "Windowed") : PendingDisplayMode == EWindowMode::WindowedFullscreen ? LOCTEXT("Borderless", "Borderless") : LOCTEXT("Fullscreen", "Fullscreen");
            Button(FText::Format(LOCTEXT("DisplayMode", "Display mode: {0} — change"), ModeName), EEchoesShellAction::WindowMode);
            Button(LOCTEXT("ApplyDisplay", "Apply display settings"), EEchoesShellAction::ApplyDisplay,
                PendingDisplayResolution != Settings->GetScreenResolution() || PendingDisplayMode != Settings->GetFullscreenMode());

            const auto Volume = [&](FText Label, float Value, EEchoesShellAction Down, EEchoesShellAction Up)
            { Button(FText::Format(LOCTEXT("VolumeDown", "{0}: {1}% — decrease"), Label, FText::AsNumber(FMath::RoundToInt(Value*100))), Down, Value > 0); Button(FText::Format(LOCTEXT("VolumeUp", "Increase {0}"), Label), Up, Value < 1); };
            Volume(LOCTEXT("Master", "Master volume"),Settings->GetMasterVolume(),EEchoesShellAction::MasterDown,EEchoesShellAction::MasterUp);
            Volume(LOCTEXT("Music", "Music"),Settings->GetMusicVolume(),EEchoesShellAction::MusicDown,EEchoesShellAction::MusicUp);
            Volume(LOCTEXT("Dialogue", "Dialogue"),Settings->GetDialogueVolume(),EEchoesShellAction::DialogueDown,EEchoesShellAction::DialogueUp);
            Volume(LOCTEXT("Effects", "Effects"),Settings->GetEffectsVolume(),EEchoesShellAction::EffectsDown,EEchoesShellAction::EffectsUp);
            Volume(LOCTEXT("Interface", "Interface"),Settings->GetInterfaceVolume(),EEchoesShellAction::InterfaceDown,EEchoesShellAction::InterfaceUp);
            Volume(LOCTEXT("Ambience", "Ambience"),Settings->GetAmbienceVolume(),EEchoesShellAction::AmbienceDown,EEchoesShellAction::AmbienceUp);
        }
        Back(); break;
    case EEchoesShellScreen::Gameplay: break;
    }
    return View;
}

void AEchoesPlayerController::HandleShellAction(EEchoesShellAction Action, int32 Argument)
{
    if (!bPlayerProfileInitialized) InitializePlayerProfile();
    const FEchoesShellView View = BuildShellView();
    if (!View.Buttons.ContainsByPredicate([&](const FEchoesShellButton& Button)
        { return Button.Action == Action && Button.Argument == Argument && Button.bEnabled; }) &&
        Action != EEchoesShellAction::Back)
        return;
    UEchoesSimulationSubsystem* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!Bridge) return;
    if (UEchoesInterfaceAudioSubsystem* Audio = GetWorld()->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
        Audio->PlayInterfaceCue(EEchoesInterfaceCue::Confirm);
    bool bConfirmed = false;
    if (Action == EEchoesShellAction::Confirm)
    {
        Action = PendingShellAction;
        Argument = PendingShellArgument;
        PlayerFlow.Back();
        ShellMessage.Reset();
        bConfirmed = true;
    }
    else if (Action == EEchoesShellAction::Retry)
    {
        if (!bPlayerProfileAvailable)
        {
            bPlayerProfileInitialized = false;
            PlayerFlow.Back();
            InitializePlayerProfile();
            RefreshShell();
            return;
        }
        Action = PendingShellAction;
        Argument = PendingShellArgument;
        PlayerFlow.Back();
        ShellMessage.Reset();
        bConfirmed = true;
    }
    if (Action == EEchoesShellAction::Back &&
        (PlayerFlow.Current() == EEchoesShellScreen::ReplayTransport || PlayerFlow.Current() == EEchoesShellScreen::ReplayBrowser))
        Action = EEchoesShellAction::ExitReplay;
    if (HandleReplayShellAction(Action, Argument, bConfirmed)) { RefreshShell(); return; }
    const auto Fail = [&](const FString& Feedback)
    {
        ShellMessage = Feedback;
        PendingShellAction = Action;
        PendingShellArgument = Argument;
        PlayerFlow.Push(EEchoesShellScreen::Error);
    };
    const auto Confirm = [&](const FText& Message)
    {
        if (bConfirmed) return true;
        PendingShellAction = Action;
        PendingShellArgument = Argument;
        ShellMessage = Message.ToString();
        PlayerFlow.Push(EEchoesShellScreen::Confirmation);
        return false;
    };
    FString Feedback;
    const auto StartTutorial = [&]()
    {
        const FEchoesPlayerProfile PriorProfile = PlayerProfile;
        PlayerProfile.bOnboardingOffered = true;
        if (!CommitPlayerProfile()) { PlayerProfile = PriorProfile; Fail(ShellMessage); return; }
        if (!Bridge->SelectOperationMode(EEchoesOperationMode::CampaignPrologue, Feedback)) { Fail(Feedback); return; }
        bTutorialOperationAuthorized = true;
        PlayerFlow.ClearOverlays();
        PresentMissionBriefing();
    };
    switch (Action)
    {
    case EEchoesShellAction::Back:
    case EEchoesShellAction::Cancel:
    {
        if (PlayerFlow.Current() == EEchoesShellScreen::DisplayConfirmation) { RevertPendingDisplay(); break; }
        const bool bCancellingProfileReset = PlayerFlow.Current() == EEchoesShellScreen::Confirmation && !bPlayerProfileAvailable;
        ShellMessage.Reset();
        if (bCancellingProfileReset) ShellMessage = LOCTEXT("ProfileStillUnavailable", "The player profile is still unavailable. Retry loading it or create a new local profile.").ToString();
        if (!PlayerFlow.Back())
        {
            if (IsPauseMenuVisible()) TogglePauseMenu();
            else if (IsMissionBriefingVisible()) { const bool bSkirmish = IsSkirmishDeploymentSummaryVisible(); PresentTitleScreen(); if (bSkirmish) PlayerFlow.Push(EEchoesShellScreen::Modes); }
            else if (IsTitleScreenVisible())
            {
                PendingShellAction = EEchoesShellAction::Quit;
                ShellMessage = LOCTEXT("QuitConfirm", "Quit Echoes of the Broken Sun?").ToString();
                PlayerFlow.Push(EEchoesShellScreen::Confirmation);
            }
        }
        break;
    }
    case EEchoesShellAction::Primary:
        if (PlayerFlow.Current() == EEchoesShellScreen::Modes)
        {
            PlayerFlow.ClearOverlays();
            ConfirmTitleScreen();
        }
        else ConfirmPrimaryAction();
        break;
    case EEchoesShellAction::Tutorial: StartTutorial(); break;
    case EEchoesShellAction::Campaign:
    case EEchoesShellAction::Modes:
        if (Action == EEchoesShellAction::Campaign && !RequireOperationMastery(EEchoesOperationMode::Skirmish))
        { Fail(ShellMessage); break; }
        if (!PlayerProfile.IsTutorialMasteryComplete() && !PlayerProfile.bTutorialOptOut)
        {
            if (!Confirm(LOCTEXT("SkipTraining", "Skip the tutorial for now? Campaign and skirmish remain locked until training is complete."))) break;
            const FEchoesPlayerProfile PriorProfile = PlayerProfile;
            PlayerProfile.bOnboardingOffered = true;
            PlayerProfile.bTutorialOptOut = true;
            if (!CommitPlayerProfile()) { PlayerProfile = PriorProfile; Fail(ShellMessage); break; }
        }
        if (!RequireOperationMastery(EEchoesOperationMode::Skirmish))
        {
            PlayerFlow.ClearOverlays();
            PresentTitleScreen();
            break;
        }
        if (Action == EEchoesShellAction::Campaign)
        {
            PlayerFlow.ClearOverlays();
            ContinueCampaign();
            if (PlayerFlow.Is(EEchoesShellScreen::Title)) ShellMessage = GetStatusMessage();
        }
        else
        {
            if (!Bridge->SelectOperationMode(EEchoesOperationMode::Skirmish, Feedback)) { Fail(Feedback); break; }
            PresentTitleScreen();
            PlayerFlow.Push(EEchoesShellScreen::Modes);
        }
        break;
    case EEchoesShellAction::Options:
        if (const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
        { PendingDisplayResolution = Settings->GetScreenResolution(); PendingDisplayMode = Settings->GetFullscreenMode(); }
        ShellMessage.Reset(); PlayerFlow.Push(EEchoesShellScreen::Options); break;
    case EEchoesShellAction::ResolutionPrevious:
    case EEchoesShellAction::ResolutionNext:
    {
        TArray<FIntPoint> Resolutions = {{1280,720},{1440,900},{1600,900},{1920,1080},{2560,1440}};
        FDisplayMetrics Metrics; FDisplayMetrics::RebuildDisplayMetrics(Metrics);
        if (Metrics.PrimaryDisplayWidth > 0 && Metrics.PrimaryDisplayHeight > 0)
            Resolutions.AddUnique(FIntPoint(Metrics.PrimaryDisplayWidth, Metrics.PrimaryDisplayHeight));
        int32 Index = Resolutions.IndexOfByKey(PendingDisplayResolution);
        if (Index == INDEX_NONE) Index = 0;
        Index = (Index + (Action == EEchoesShellAction::ResolutionNext ? 1 : Resolutions.Num()-1)) % Resolutions.Num();
        PendingDisplayResolution = Resolutions[Index];
        break;
    }
    case EEchoesShellAction::WindowMode:
        PendingDisplayMode = PendingDisplayMode == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen
            : PendingDisplayMode == EWindowMode::WindowedFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed;
        break;
    case EEchoesShellAction::ApplyDisplay:
        if (UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
        {
            PreviousDisplayResolution = Settings->GetScreenResolution(); PreviousDisplayMode = Settings->GetFullscreenMode();
            Settings->SetScreenResolution(PendingDisplayResolution); Settings->SetFullscreenMode(PendingDisplayMode);
            if (!FApp::IsUnattended() && GetWorld()->WorldType != EWorldType::PIE) Settings->ApplyResolutionSettings(false);
            DisplayRevertDeadline = FPlatformTime::Seconds() + 15.0;
            PlayerFlow.Push(EEchoesShellScreen::DisplayConfirmation);
        }
        break;
    case EEchoesShellAction::RevertDisplay: RevertPendingDisplay(); break;
    case EEchoesShellAction::KeepDisplay:
        if (!CommitPlayerProfile())
        {
            const FString Error = ShellMessage;
            const FIntPoint RequestedResolution = PendingDisplayResolution;
            const EWindowMode::Type RequestedMode = PendingDisplayMode;
            RevertPendingDisplay();
            PendingDisplayResolution = RequestedResolution;
            PendingDisplayMode = RequestedMode;
            Fail(Error);
            PendingShellAction = EEchoesShellAction::ApplyDisplay;
        }
        else
        {
            DisplayRevertDeadline = 0;
            PlayerFlow.Back();
            if (auto* Settings = UEchoesGameUserSettings::Get())
            {
                Settings->ConfirmVideoMode();
                Settings->SaveSettings();
            }
        }
        break;
    case EEchoesShellAction::SaveLoad: ShellMessage.Reset(); PlayerFlow.Push(EEchoesShellScreen::SaveLoad); break;
    case EEchoesShellAction::Credits: ShellMessage.Reset(); PlayerFlow.Push(EEchoesShellScreen::Credits); break;
    case EEchoesShellAction::PreviousSetting: FocusPreviousSkirmishSetting(); break;
    case EEchoesShellAction::NextSetting: FocusNextSkirmishSetting(); break;
    case EEchoesShellAction::DecreaseSetting: DecreaseSkirmishSetting(); break;
    case EEchoesShellAction::IncreaseSetting: IncreaseSkirmishSetting(); break;
    case EEchoesShellAction::NextOperation: CycleOperation(); break;
    case EEchoesShellAction::Resume: PlayerFlow.ClearOverlays(); if (IsPauseMenuVisible()) TogglePauseMenu(); break;
    case EEchoesShellAction::Save:
        if (!Bridge->RequestQuickSaveScenario(Feedback)) Fail(Feedback); else ShellMessage = Feedback;
        break;
    case EEchoesShellAction::Load:
    case EEchoesShellAction::Recover:
    {
        if (!Confirm(LOCTEXT("LoadConfirm", "Restore the saved session? Progress since the last save will be replaced."))) break;
        FEchoesRecoveryCandidate Recovery;
        if (Action == EEchoesShellAction::Recover && !Bridge->CheckInterruptedSessionRecovery(Recovery, Feedback))
        { Fail(Feedback); break; }
        const EEchoesOperationMode RestoredOperation = Action == EEchoesShellAction::Load
            ? Bridge->GetOperationMode() : Recovery.OperationMode;
        if (!RequireOperationMastery(RestoredOperation, true)) { Fail(ShellMessage); break; }
        if (!(Action == EEchoesShellAction::Load ? Bridge->QuickLoadScenario(Feedback) : Bridge->RecoverInterruptedSession(Recovery, Feedback)))
            Fail(Feedback);
        else
        {
            PlayerFlow.ClearOverlays();
            if (Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue)
                bTutorialOperationAuthorized = true;
            if (!RequireOperationMastery(Bridge->GetOperationMode()))
            {
                PresentMissionBriefing();
                break;
            }
            PlayerFlow.SetVisible(PlayerFlow.BaseScreen(), false);
            Bridge->SetScenarioPaused(false);
            ResetIgnoreMoveInput(); ResetIgnoreLookInput();
            ShellMessage = Feedback;
        }
        break;
    }
    case EEchoesShellAction::SelectSlot:
    {
        const uint8 PriorSlot = PlayerProfile.ActiveJourneySlot;
        if (!Bridge->SelectJourneySlot(Argument, Feedback)) { Fail(Feedback); break; }
        PlayerProfile.ActiveJourneySlot = static_cast<uint8>(Argument);
        if (!CommitPlayerProfile())
        {
            PlayerProfile.ActiveJourneySlot = PriorSlot;
            FString RollbackFeedback;
            Bridge->SelectJourneySlot(PriorSlot, RollbackFeedback);
            Fail(ShellMessage); break;
        }
        PresentTitleScreen(); PlayerFlow.Push(EEchoesShellScreen::SaveLoad);
        ShellMessage = Feedback;
        break;
    }
    case EEchoesShellAction::NewJourney:
    case EEchoesShellAction::RestoreJourney:
        if (!Confirm(Action == EEchoesShellAction::NewJourney
            ? LOCTEXT("NewConfirm", "Start this journey again? The current ledger will be retained as its previous generation.")
            : LOCTEXT("RestoreConfirm", "Restore this slot's previous journey? The current ledger will become its backup."))) break;
        if (!(Action == EEchoesShellAction::NewJourney ? Bridge->StartNewCampaign(Feedback) : Bridge->RestoreCampaignBackup(Feedback))) Fail(Feedback);
        else { PresentTitleScreen(); PlayerFlow.Push(EEchoesShellScreen::SaveLoad); ShellMessage = Feedback; }
        break;
    case EEchoesShellAction::Restart:
        if (Confirm(LOCTEXT("RestartConfirm", "Restart the current mission? Unsaved progress will be lost.")))
        { PlayerFlow.ClearOverlays(); RestartScenario(); }
        break;
    case EEchoesShellAction::Concede:
        if (!Confirm(LOCTEXT("ConcedeConfirm", "Concede this match and record a defeat?"))) break;
        if (!Bridge->ConcedeOfflineMatch(Feedback)) Fail(Feedback);
        else { PlayerFlow.ClearOverlays(); NotifyMatchFinished(Bridge->GetMatchOutcome()); }
        break;
    case EEchoesShellAction::ReturnToMenu:
        if (Confirm(LOCTEXT("MenuConfirm", "Return to the main menu? Unsaved progress will be lost.")))
        { PlayerFlow.ClearOverlays(); PresentTitleScreen(); }
        break;
    case EEchoesShellAction::ResetProfile:
        if (!Confirm(LOCTEXT("ResetProfileConfirm", "Create a new local profile? Unreadable profile files will be archived. Your journey saves will be preserved."))) break;
        {
            FEchoesPlayerProfile Defaults;
            Defaults.ActiveJourneySlot = static_cast<uint8>(Bridge->GetActiveJourneySlot());
            if (const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get()) Defaults.CaptureSettings(*Settings);
            if (!FEchoesPlayerProfileStore::ResetPreservingInvalidGenerations(
                FEchoesPlayerProfileStore::GetDefaultPath(), Defaults, Feedback)) { Fail(Feedback); break; }
            bPlayerProfileInitialized = false;
            PlayerFlow.ClearOverlays();
            InitializePlayerProfile();
        }
        break;
    case EEchoesShellAction::Quit:
        if (Confirm(LOCTEXT("QuitConfirm", "Quit Echoes of the Broken Sun?")))
            UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
        break;
    default:
    {
        UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
        if (!Settings) { Fail(TEXT("Settings are unavailable.")); break; }
        const auto Volume = [&](EEchoesAudioCategory Category, float Delta)
        { Settings->SetAudioCategoryVolume(Category, FMath::Clamp(Settings->GetAudioCategoryVolume(Category) + Delta, 0.0f, 1.0f)); };
        switch (Action)
        {
        case EEchoesShellAction::HudScaleDown: Settings->SetHudScale(Settings->GetHudScale() - .05f); break;
        case EEchoesShellAction::HudScaleUp: Settings->SetHudScale(Settings->GetHudScale() + .05f); break;
        case EEchoesShellAction::HighContrast: Settings->SetHighContrastHudEnabled(!Settings->IsHighContrastHudEnabled()); break;
        case EEchoesShellAction::ReducedMotion: Settings->SetReducedMotionEnabled(!Settings->IsReducedMotionEnabled()); break;
        case EEchoesShellAction::ReducedFlashing: Settings->SetReducedFlashingEnabled(!Settings->IsReducedFlashingEnabled()); break;
        case EEchoesShellAction::EdgePan: Settings->SetEdgePanEnabled(!Settings->IsEdgePanEnabled()); break;
        case EEchoesShellAction::CameraPanDown: Settings->SetCameraPanSpeedScale(Settings->GetCameraPanSpeedScale() - .1f); break;
        case EEchoesShellAction::CameraPanUp: Settings->SetCameraPanSpeedScale(Settings->GetCameraPanSpeedScale() + .1f); break;
        case EEchoesShellAction::CameraZoomDown: Settings->SetCameraZoomScale(Settings->GetCameraZoomScale() - .1f); break;
        case EEchoesShellAction::CameraZoomUp: Settings->SetCameraZoomScale(Settings->GetCameraZoomScale() + .1f); break;
        case EEchoesShellAction::DynamicRange: Settings->SetReducedDynamicRangeEnabled(!Settings->IsReducedDynamicRangeEnabled()); break;
        case EEchoesShellAction::MasterDown: Settings->SetMasterVolume(FMath::Max(0.f, Settings->GetMasterVolume()-.1f)); break;
        case EEchoesShellAction::MasterUp: Settings->SetMasterVolume(FMath::Min(1.f, Settings->GetMasterVolume()+.1f)); break;
        case EEchoesShellAction::MusicDown: Volume(EEchoesAudioCategory::Music,-.1f); break;
        case EEchoesShellAction::MusicUp: Volume(EEchoesAudioCategory::Music,.1f); break;
        case EEchoesShellAction::DialogueDown: Volume(EEchoesAudioCategory::Dialogue,-.1f); break;
        case EEchoesShellAction::DialogueUp: Volume(EEchoesAudioCategory::Dialogue,.1f); break;
        case EEchoesShellAction::EffectsDown: Volume(EEchoesAudioCategory::Effects,-.1f); break;
        case EEchoesShellAction::EffectsUp: Volume(EEchoesAudioCategory::Effects,.1f); break;
        case EEchoesShellAction::InterfaceDown: Volume(EEchoesAudioCategory::Interface,-.1f); break;
        case EEchoesShellAction::InterfaceUp: Volume(EEchoesAudioCategory::Interface,.1f); break;
        case EEchoesShellAction::AmbienceDown: Volume(EEchoesAudioCategory::Ambience,-.1f); break;
        case EEchoesShellAction::AmbienceUp: Volume(EEchoesAudioCategory::Ambience,.1f); break;
        default: return;
        }
        if (!CommitPlayerProfile())
        {
            FString RestoreError;
            PlayerProfile.ApplySettings(*Settings, RestoreError);
            Fail(ShellMessage);
        }
        break;
    }
    }
    RefreshShell();
}

void AEchoesPlayerController::RevertPendingDisplay()
{
    if (DisplayRevertDeadline <= 0.0) return;
    if (auto* Settings = UEchoesGameUserSettings::Get())
    {
        Settings->SetScreenResolution(PreviousDisplayResolution);
        Settings->SetFullscreenMode(PreviousDisplayMode);
        if (!FApp::IsUnattended() && GetWorld() && GetWorld()->WorldType != EWorldType::PIE) Settings->ApplyResolutionSettings(false);
    }
    PendingDisplayResolution = PreviousDisplayResolution; PendingDisplayMode = PreviousDisplayMode;
    DisplayRevertDeadline = 0;
    if (PlayerFlow.Current() == EEchoesShellScreen::DisplayConfirmation) PlayerFlow.Back();
}

void AEchoesPlayerController::HandleShellValue(EEchoesShellAction Action, float Value, bool bCommit)
{
    if (Action == EEchoesShellAction::ReplaySeek && FMath::IsFinite(Value) &&
        PlayerFlow.Current() == EEchoesShellScreen::ReplayTransport)
    {
        if (auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr; Bridge && bCommit)
        {
            const auto* Metadata = Bridge->GetActiveReplayMetadata();
            const auto State = Bridge->GetReplayPlaybackState();
            const uint64 Requested = FMath::Clamp<uint64>(static_cast<uint64>(FMath::Clamp(Value, 0.f, 1.f) * State.FinalTick),
                Metadata ? Metadata->CoverageStartTick : 0, State.FinalTick);
            FString Feedback;
            if (!Bridge->SeekReplayTick(Requested, Feedback))
            {
                ShellMessage = Feedback;
                PendingReplayTick = Requested;
                PendingShellAction = EEchoesShellAction::ReplaySeek;
                PendingShellArgument = 0;
                PlayerFlow.Push(EEchoesShellScreen::Error);
            }
            RefreshShell();
        }
        return;
    }
    if (!FMath::IsFinite(Value) || PlayerFlow.Current() != EEchoesShellScreen::Options ||
        Action != EEchoesShellAction::HudScaleValue) return;
    if (auto* Settings = UEchoesGameUserSettings::Get())
    {
        Settings->SetHudScale(Value);
        if (bCommit)
        {
            if (!CommitPlayerProfile())
            {
                FString Error; PlayerProfile.ApplySettings(*Settings, Error);
                PendingShellAction = EEchoesShellAction::Options;
                PlayerFlow.Push(EEchoesShellScreen::Error);
            }
            RefreshShell();
        }
    }
}

#undef LOCTEXT_NAMESPACE
