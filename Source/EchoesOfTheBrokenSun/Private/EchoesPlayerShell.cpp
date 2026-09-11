#include "EchoesPlayerController.h"
#include "EchoesCheckpointFeedback.h"
#include "EchoesShellWidget.h"
#include "EchoesInputPrompt.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesCampaignProgress.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesInterfaceAudioSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "GenericPlatform/GenericApplication.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "EchoesPlayerShell"

namespace
{
/**
 * What a journey slot holds, read without selecting it. The three rows read
 * "Select Slot 1/2/3" and nothing else: a player could not tell an empty slot
 * from the one carrying forty decisions, and the only way to find out was to
 * commit to it. Reading is side-effect free -- no copy, no migration, no
 * change of active slot -- so an unreadable slot reports that it needs
 * recovery rather than being silently replaced.
 */
FText DescribeJourneySlot(int32 Slot)
{
    const FString Path = UEchoesSimulationSubsystem::GetJourneySlotPath(Slot);
    if (Path.IsEmpty())
    {
        return LOCTEXT("SlotPathUnavailable", "unavailable");
    }
    if (!IFileManager::Get().FileExists(*Path))
    {
        return LOCTEXT("SlotEmpty", "empty — a new journey starts here");
    }
    FEchoesCampaignProgress Progress;
    FString Feedback;
    if (!FEchoesCampaignProgressStore::LoadWithBackup(Path, Progress, Feedback))
    {
        return LOCTEXT("SlotUnreadable",
            "unreadable — choose Restore previous journey to recover it");
    }
    int32 FurthestMission = 0;
    for (const FEchoesCampaignDecisionRecord& Decision : Progress.Decisions)
    {
        FurthestMission = FMath::Max(
            FurthestMission, static_cast<int32>(Decision.Mission));
    }
    const FDateTime Written = IFileManager::Get().GetTimeStamp(*Path);
    const FText When = Written == FDateTime::MinValue()
        ? LOCTEXT("SlotTimeUnknown", "date unknown")
        : FText::AsDateTime(Written);
    if (Progress.Decisions.IsEmpty())
    {
        return FText::Format(
            LOCTEXT("SlotStarted", "started, no decisions recorded yet · {0}"),
            When);
    }
    return FText::Format(
        LOCTEXT("SlotProgress", "mission {0} of {1} · {2} decisions · {3}"),
        FText::AsNumber(FurthestMission),
        FText::AsNumber(static_cast<int32>(
            EEchoesCampaignMissionId::TheBrokenSun)),
        FText::AsNumber(Progress.Decisions.Num()),
        When);
}

// AssetRegister.md requires placeholders to remain visibly and textually labeled in
// development builds and never to be described as final art. The vertical-slice
// meshes, materials, lighting and effects currently on screen are first-pass work,
// so every build that can still contain them says so where a player reads it.
//
// Two gates, because one is not sufficient. The compile-time gate keeps the string
// out of a Shipping binary entirely, so a release build audited under REL-GOV-015
// and DEMO-VIS-010 has never contained development language. The runtime opt-out
// exists because Scripts/package_macos.sh hard-codes -clientconfig=Development and
// Scripts/verify_packaged_app.py refuses any other configuration: today every
// artifact the accepted pipeline can produce is a Development build, so a
// compile-time gate alone would leave no way to take an owner acceptance capture
// without this text on screen. Showing it is the default, because a forgotten flag
// then leaves a true statement visible, where defaulting to hidden would let an
// unfinished build present itself as finished.
//
// This discloses an unfinished visual state; it does not cure one. It must stop
// being true - by the art being finished - before DEMO-VIS-010 or REL-GOV-015 can
// be claimed.
FText PreReleaseArtNotice()
{
#if UE_BUILD_SHIPPING
    return FText::GetEmpty();
#else
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesFinalArtPath")))
        return FText::GetEmpty();
    return LOCTEXT("PreReleaseArtNotice",
        "The art and graphics are not finished. Visual polish is scheduled for the final phase of production.");
#endif
}
}

bool AEchoesPlayerController::RequireOperationProfile()
{
    // A non-player controller used by runtime fixtures has no profile authority.
    // Every local player, and any controller exercising the profile flow, does.
    if (GetLocalPlayer() == nullptr && !bPlayerProfileInitialized) return true;
    if (!bPlayerProfileInitialized && !InitializePlayerProfile()) return false;
    // SPEC-TUT-008: learning progress is guidance, never permission to play.
    // Keep the independent profile-recovery boundary for every local player.
    if (bPlayerProfileAvailable) return true;
    ShellMessage = LOCTEXT("ProfileRequiredForDeployment", "Recover your player profile before deploying.").ToString();
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
            // Startup must honor explicit window/resolution launch overrides.
            // In-session options retain their separate user-confirmation flow.
            else Settings->ApplySettings(true);
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

bool AEchoesPlayerController::IsOnlineLocalMenuShellRouteActive() const
{
    if (!bOnlineLocalMenuVisible || !IsActiveOnlineNetworkMatch() ||
        IsReplayInputActive())
    {
        return false;
    }

    switch (PlayerFlow.Current())
    {
        case EEchoesShellScreen::Options:
        case EEchoesShellScreen::Controls:
        case EEchoesShellScreen::ControlCapture:
        case EEchoesShellScreen::FeedbackHistory:
        case EEchoesShellScreen::ResourceMonitor:
        case EEchoesShellScreen::Confirmation:
        case EEchoesShellScreen::DisplayConfirmation:
        case EEchoesShellScreen::Error:
            return true;
        default:
            return false;
    }
}

bool AEchoesPlayerController::OpenOnlineLocalMenuShellScreen(
    EEchoesShellScreen Screen)
{
    if (!bOnlineLocalMenuVisible || !IsActiveOnlineNetworkMatch() ||
        IsReplayInputActive() ||
        PlayerFlow.Current() != EEchoesShellScreen::Gameplay)
    {
        return false;
    }

    switch (Screen)
    {
        case EEchoesShellScreen::Options:
            SeedPendingDisplayFromLivePresentation();
            ShellMessage.Reset();
            PlayerFlow.Push(Screen);
            break;
        case EEchoesShellScreen::Controls:
            // Do not clear ControlBindingMessage here. The online route stores the
            // match-continuity warning in it before pushing, and that warning is
            // what BuildControlsShellView publishes as the screen's status line.
            PlayerFlow.Push(Screen);
            break;
        case EEchoesShellScreen::ResourceMonitor:
            if (!OpenResourceMonitor()) return false;
            break;
        case EEchoesShellScreen::FeedbackHistory:
            if (!HandleFeedbackHistoryShellAction(
                    EEchoesShellAction::OpenFeedbackHistory))
            {
                return false;
            }
            break;
        default:
            return false;
    }

    // This only transfers local input to a UMG route. The online menu itself
    // deliberately leaves scenario authority running.
    RefreshShell();
    return true;
}

bool AEchoesPlayerController::UsesShellWidget() const
{
    const bool bOnlineFieldMenu = bOnlineLocalMenuVisible &&
        PlayerFlow.Current() == EEchoesShellScreen::Gameplay;
    return ShellWidget != nullptr && ShellWidget->IsVisible() &&
        PlayerFlow.Current() != EEchoesShellScreen::Gameplay && PlayerFlow.Current() != EEchoesShellScreen::ReplayTransport &&
        !IsOnlineFrontDoorVisible() && !bCampaignOperationsMapVisible && !bOnlineFieldMenu;
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
    const bool bOnlineFieldMenu = bOnlineLocalMenuVisible &&
        PlayerFlow.Current() == EEchoesShellScreen::Gameplay;
    const bool bShow = PlayerFlow.Current() != EEchoesShellScreen::Gameplay &&
        !IsOnlineFrontDoorVisible() && !bCampaignOperationsMapVisible && !bOnlineFieldMenu;
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
    case EEchoesShellScreen::Controls:
    case EEchoesShellScreen::ControlCapture:
    {
        FEchoesShellView ControlsView = BuildControlsShellView();
        if (IsOnlineLocalMenuShellRouteActive())
        {
            const FText Continuity = LOCTEXT("OnlineControlsContinuity",
                "ONLINE MATCH CONTINUES — local controls are held while this menu is open.");
            ControlsView.Status = ControlsView.Status.IsEmpty()
                ? Continuity
                : FText::Format(LOCTEXT("OnlineControlsStatus", "{0}\n{1}"),
                    Continuity, ControlsView.Status);
        }
        return ControlsView;
    }
    case EEchoesShellScreen::ResourceMonitor:
        BuildResourceMonitorShellView(View);
        break;
    case EEchoesShellScreen::FeedbackHistory:
        BuildFeedbackHistoryShellView(View);
        break;
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
        Button(LOCTEXT("Help", "Help and lesson practice"), EEchoesShellAction::Help);
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
        // REL-UI-010. This screen was the operation label, one hardcoded
        // sentence chosen from a sixteen-way chain, and a Deploy button, while
        // the authored briefing and objectives for every operation sat unread
        // in the narrative pack. It is the only screen between the front door
        // and sixteen campaign missions, so a player deployed into authored
        // content nobody had told them anything about.
        const EEchoesOperationMode BriefingMode = Bridge
            ? Bridge->GetOperationMode() : EEchoesOperationMode::Skirmish;
        UGameInstance* BriefingGameInstance =
            GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
        const UEchoesNarrativeSubsystem* Narrative =
            BriefingGameInstance != nullptr
                ? BriefingGameInstance->GetSubsystem<UEchoesNarrativeSubsystem>()
                : nullptr;
        const FString AuthoredTitle =
            Narrative != nullptr ? Narrative->GetTitle(BriefingMode) : FString();
        View.Title = !AuthoredTitle.IsEmpty()
            ? FText::FromString(AuthoredTitle)
            : (Bridge ? FText::FromString(Bridge->GetOperationLabel())
                      : LOCTEXT("Brief", "Mission briefing"));

        TArray<FString> BriefingLines;
        const FString AuthoredBriefing =
            Narrative != nullptr ? Narrative->GetBriefing(BriefingMode) : FString();
        if (!AuthoredBriefing.IsEmpty())
        {
            BriefingLines.Add(AuthoredBriefing);
        }
        const TArray<FString> Objectives = Narrative != nullptr
            ? Narrative->GetObjectives(BriefingMode) : TArray<FString>();
        if (!Objectives.IsEmpty())
        {
            BriefingLines.Add(FString());
            BriefingLines.Add(LOCTEXT("BriefingObjectives", "OBJECTIVES").ToString());
            for (int32 Index = 0; Index < Objectives.Num(); ++Index)
            {
                BriefingLines.Add(FString::Printf(
                    TEXT("%d. %s"), Index + 1, *Objectives[Index]));
            }
        }
        // An irreversible decision is warned about before the player can walk
        // into it, not after. A Future Well protocol is committed once and the
        // campaign ledger refuses to rewrite that mission's choice, so the
        // warning is raised from the staged scenario rather than a per-mission
        // list that would fall out of date.
        // Only a campaign mission writes the journey ledger. The readiness
        // drill shares M01's map, Future Well and all, but SPEC-TUT-008.COVERAGE
        // forbids training from altering campaign state -- so warning there
        // would be telling the player something untrue about their save.
        static_assert(
            static_cast<uint8>(EEchoesOperationMode::CampaignPrologue) <
                static_cast<uint8>(EEchoesOperationMode::CampaignTheBrokenSun) &&
            static_cast<uint8>(EEchoesOperationMode::TrainingReadiness) >
                static_cast<uint8>(EEchoesOperationMode::CampaignTheBrokenSun),
            "the campaign operations must stay one contiguous range");
        const bool bLedgerMission =
            BriefingMode >= EEchoesOperationMode::CampaignPrologue &&
            BriefingMode <= EEchoesOperationMode::CampaignTheBrokenSun;
        bool bCarriesIrreversibleChoice = false;
        if (bLedgerMission && Bridge != nullptr && Bridge->IsScenarioReady())
        {
            if (const auto* BriefingSimulation = Bridge->GetSimulation())
            {
                for (const auto& Entity : BriefingSimulation->Entities())
                {
                    if (Entity.type == echoes::sim::EntityType::FutureWell)
                    {
                        bCarriesIrreversibleChoice = true;
                        break;
                    }
                }
            }
        }
        if (bCarriesIrreversibleChoice)
        {
            BriefingLines.Add(FString());
            BriefingLines.Add(LOCTEXT("BriefingIrreversible",
                "IRREVERSIBLE DECISION — this operation holds a Future Well. Committing a protocol (Harvest, Preserve or Reshape) is permanent: the choice is written to this journey's ledger and cannot be retaken on a later attempt at this mission.")
                .ToString());
        }
        const FString BriefingStatus = GetStatusMessage();
        if (!BriefingStatus.IsEmpty())
        {
            BriefingLines.Add(FString());
            BriefingLines.Add(BriefingStatus);
        }
        View.Body = BriefingLines.IsEmpty()
            ? FText::FromString(BriefingStatus)
            : FText::FromString(FString::Join(BriefingLines, TEXT("\n")));
        Button(LOCTEXT("Deploy", "Deploy"), EEchoesShellAction::Primary,
            bPlayerProfileAvailable && Bridge && Bridge->IsScenarioReady());
        Back(); break;
    }
    case EEchoesShellScreen::Pause:
        View.Title = LOCTEXT("Paused", "Paused");
        View.Body = LOCTEXT("PauseBody", "The battlefield is held. Resume when you are ready.");
        Button(LOCTEXT("Resume", "Resume"), EEchoesShellAction::Resume);
        Button(LOCTEXT("ResourceMonitor", "Resource monitor"), EEchoesShellAction::OpenResourceMonitor);
        Button(LOCTEXT("CommandHistory", "Command history"), EEchoesShellAction::OpenFeedbackHistory);
        Button(LOCTEXT("Options", "Options"), EEchoesShellAction::Options);
        Button(LOCTEXT("Help", "Help and lesson practice"), EEchoesShellAction::Help);
        Button(LOCTEXT("SaveLoad", "Save and load"), EEchoesShellAction::SaveLoad);
        Button(LOCTEXT("Restart", "Restart mission"), EEchoesShellAction::Restart);
        // SPEC-OUT-007: the player may concede at any time. A campaign operation going
        // badly previously had no exit but winning or losing it, and losing depends on
        // the opponent finishing the job. TrainingReadiness keeps its own opt-out and is
        // not concedable, so the label distinguishes an operation from a skirmish match.
        if (Bridge && Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness)
            Button(Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish
                    ? LOCTEXT("Concede", "Concede match")
                    : LOCTEXT("ConcedeOperation", "Concede operation"),
                EEchoesShellAction::Concede);
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
        {
            // Slots are chosen from the title screen only, because selecting one
            // swaps the active campaign ledger. Saying so turns three greyed
            // rows from something that looks broken into something deliberate.
            const bool bSlotsSelectable =
                PlayerFlow.BaseScreen() == EEchoesShellScreen::Title;
            if (!bSlotsSelectable)
            {
                View.Body = FText::Format(
                    LOCTEXT("SlotBodyLocked", "{0}\n\nJourneys can only be switched from the title screen; leave this operation first."),
                    View.Body);
            }
            for (int32 Slot = 1; Slot <= 3; ++Slot)
            {
                Button(
                    FText::Format(
                        LOCTEXT("SlotWithContents", "Slot {0} — {1}"),
                        FText::AsNumber(Slot), DescribeJourneySlot(Slot)),
                    EEchoesShellAction::SelectSlot, bSlotsSelectable, Slot);
            }
        }
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
            View.Status = EchoesCheckpointFeedback::Display(Bridge->GetCheckpointSaveStatus());
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
    case EEchoesShellScreen::Help:
    {
        View.Title = LOCTEXT("HelpTitle", "Help and lesson practice");
        View.Body = FText::Format(LOCTEXT("HelpActiveControls",
            "Select: {0}   Context order: {1}\nCamera: {2} / {3}   Zoom: {4} / {5}\nAttack-move: {6}   Hold: {7}   Stop: {8}\n\nChoose a readiness lesson to practice. Your saved progress stays intact."),
            FEchoesInputPrompt::Action(TEXT("Select")),
            FEchoesInputPrompt::Action(TEXT("ContextOrder")),
            FEchoesInputPrompt::Axis(TEXT("CameraForward")),
            FEchoesInputPrompt::Axis(TEXT("CameraRight")),
            FEchoesInputPrompt::Action(TEXT("CameraZoomIn")),
            FEchoesInputPrompt::Action(TEXT("CameraZoomOut")),
            FEchoesInputPrompt::Action(TEXT("AttackMoveAtCursor")),
            FEchoesInputPrompt::Action(TEXT("HoldSelected")),
            FEchoesInputPrompt::Action(TEXT("StopSelected")));
        static const TCHAR* LessonLabels[] = {
            TEXT("Survey"), TEXT("Roster"), TEXT("Muster"),
            TEXT("Route"), TEXT("Reserve"), TEXT("Link restoration"),
            TEXT("Array Foundry"), TEXT("Probe"), TEXT("Board"),
            TEXT("Future Well")};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(LessonLabels); ++Index)
        {
            const uint16 Bit = static_cast<uint16>(1u << Index);
            const bool bImplemented =
                (Bit & FEchoesTutorialPracticeState::ImplementedLessonMask) != 0;
            const bool bMastered =
                (PlayerProfile.TutorialVerifiedMask & Bit) != 0;
            const FString Suffix = !bImplemented ? TEXT(" — unavailable")
                : bMastered ? TEXT(" — mastered; practice again")
                            : TEXT(" — practice");
            Button(FText::FromString(FString::Printf(TEXT("%s%s"),
                    LessonLabels[Index], *Suffix)),
                EEchoesShellAction::PracticeTutorialLesson,
                bImplemented, Bit);
        }
        Back();
        break;
    }
    case EEchoesShellScreen::DisplayConfirmation:
    {
        View.Title = LOCTEXT("KeepDisplayTitle", "Keep these display settings?");
        // SPEC-UI-009 requires the timeout itself to be displayed. RefreshShell rebuilds
        // this view every PlayerTick, so the remaining wall time counts down on screen
        // instead of restating a fixed fifteen seconds that may already have elapsed.
        const double RemainingSeconds = DisplayRevertDeadline > 0.0
            ? DisplayRevertDeadline - FPlatformTime::Seconds() : 15.0;
        View.Body = FText::Format(
            LOCTEXT("KeepDisplayBody", "Choose Keep to save this display mode. It reverts automatically in {0} seconds."),
            FText::AsNumber(FMath::Clamp(FMath::CeilToInt(RemainingSeconds), 0, 15)));
        Button(LOCTEXT("RevertDisplay", "Revert"), EEchoesShellAction::RevertDisplay);
        Button(LOCTEXT("KeepDisplay", "Keep"), EEchoesShellAction::KeepDisplay);
        break;
    }
    case EEchoesShellScreen::Options:
        View.Title = LOCTEXT("Options", "Options");
        View.Body = LOCTEXT("OptionsBody", "Audio, camera and UI changes are saved immediately. Apply a display change, then choose Keep.");
        if (Settings)
        {
            View.Sliders.Add({FText::Format(LOCTEXT("ScaleSlider", "UI scale: {0}%"), FText::AsNumber(FMath::RoundToInt(Settings->GetHudScale()*100))), EEchoesShellAction::HudScaleValue, Settings->GetHudScale(), .8f, 1.5f});
            Button(FText::Format(LOCTEXT("ScaleDown", "UI scale: {0}% — decrease"), FText::AsNumber(FMath::RoundToInt(Settings->GetHudScale()*100))), EEchoesShellAction::HudScaleDown, Settings->GetHudScale() > .8f + KINDA_SMALL_NUMBER);
            Button(LOCTEXT("ScaleUp", "Increase UI scale"), EEchoesShellAction::HudScaleUp, Settings->GetHudScale() < 1.5f - KINDA_SMALL_NUMBER);
            const auto Toggle = [&](FText Label, bool bOn, EEchoesShellAction Action)
            { Button(FText::Format(LOCTEXT("Toggle", "{0}: {1}"), Label, bOn ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off")), Action); };
            Toggle(LOCTEXT("Contrast", "High contrast"), Settings->IsHighContrastHudEnabled(), EEchoesShellAction::HighContrast);
            Toggle(LOCTEXT("Motion", "Reduced motion"), Settings->IsReducedMotionEnabled(), EEchoesShellAction::ReducedMotion);
            Toggle(LOCTEXT("Flashing", "Reduced flashing"), Settings->IsReducedFlashingEnabled(), EEchoesShellAction::ReducedFlashing);
            Button(LOCTEXT("Controls", "Controls and key bindings"), EEchoesShellAction::OpenControls);
            Toggle(LOCTEXT("EdgePan", "Edge pan"), Settings->IsEdgePanEnabled(), EEchoesShellAction::EdgePan);
            Button(FText::Format(LOCTEXT("PanDown", "Camera pan speed: {0}% — decrease"), FText::AsNumber(FMath::RoundToInt(Settings->GetCameraPanSpeedScale()*100))), EEchoesShellAction::CameraPanDown, Settings->GetCameraPanSpeedScale() > .5f);
            Button(LOCTEXT("PanUp", "Increase camera pan speed"), EEchoesShellAction::CameraPanUp, Settings->GetCameraPanSpeedScale() < 2.f);
            Button(FText::Format(LOCTEXT("ZoomDown", "Camera zoom step: {0}% — decrease"), FText::AsNumber(FMath::RoundToInt(Settings->GetCameraZoomScale()*100))), EEchoesShellAction::CameraZoomDown, Settings->GetCameraZoomScale() > .5f);
            Button(LOCTEXT("ZoomUp", "Increase camera zoom step"), EEchoesShellAction::CameraZoomUp, Settings->GetCameraZoomScale() < 2.f);
            Button(FText::Format(LOCTEXT("ResolutionDown", "Resolution: {0} × {1} — previous"), FText::AsNumber(PendingDisplayResolution.X, &FNumberFormattingOptions::DefaultNoGrouping()), FText::AsNumber(PendingDisplayResolution.Y, &FNumberFormattingOptions::DefaultNoGrouping())), EEchoesShellAction::ResolutionPrevious);
            Button(LOCTEXT("ResolutionUp", "Next resolution"), EEchoesShellAction::ResolutionNext);
            const FText ModeName = PendingDisplayMode == EWindowMode::Windowed ? LOCTEXT("Windowed", "Windowed") : PendingDisplayMode == EWindowMode::WindowedFullscreen ? LOCTEXT("Borderless", "Borderless") : LOCTEXT("Fullscreen", "Fullscreen");
            Button(FText::Format(LOCTEXT("DisplayMode", "Display mode: {0} — change"), ModeName), EEchoesShellAction::WindowMode);
            // Apply is also offered when the stored settings do not describe the window
            // on screen. Without that, a player whose window mode disagrees with the
            // saved preference sees a greyed-out Apply and has no route back to a
            // presentation that matches what Options reports.
            FIntPoint LiveResolution = Settings->GetScreenResolution();
            EWindowMode::Type LiveMode = Settings->GetFullscreenMode();
            const bool bSettingsDescribeWindow =
                !GetLiveDisplayPresentation(LiveResolution, LiveMode) ||
                (LiveResolution == Settings->GetScreenResolution() &&
                 LiveMode == Settings->GetFullscreenMode());
            Button(LOCTEXT("ApplyDisplay", "Apply display settings"), EEchoesShellAction::ApplyDisplay,
                PendingDisplayResolution != Settings->GetScreenResolution() ||
                PendingDisplayMode != Settings->GetFullscreenMode() || !bSettingsDescribeWindow);

            Toggle(LOCTEXT("DynamicRange", "Reduced dynamic range"), Settings->IsReducedDynamicRangeEnabled(), EEchoesShellAction::DynamicRange);

            const auto Volume = [&](FText Label, float Value, EEchoesShellAction Down, EEchoesShellAction Up)
            { Button(FText::Format(LOCTEXT("VolumeDown", "{0}: {1}% — decrease"), Label, FText::AsNumber(FMath::RoundToInt(Value*100))), Down, Value > 0); Button(FText::Format(LOCTEXT("VolumeUp", "Increase {0}"), Label), Up, Value < 1); };
            Volume(LOCTEXT("Master", "Master volume"),Settings->GetMasterVolume(),EEchoesShellAction::MasterDown,EEchoesShellAction::MasterUp);
            Volume(LOCTEXT("Music", "Music"),Settings->GetMusicVolume(),EEchoesShellAction::MusicDown,EEchoesShellAction::MusicUp);
            Volume(LOCTEXT("Dialogue", "Dialogue"),Settings->GetDialogueVolume(),EEchoesShellAction::DialogueDown,EEchoesShellAction::DialogueUp);
            Volume(LOCTEXT("Effects", "Effects"),Settings->GetEffectsVolume(),EEchoesShellAction::EffectsDown,EEchoesShellAction::EffectsUp);
            Volume(LOCTEXT("Interface", "Interface"),Settings->GetInterfaceVolume(),EEchoesShellAction::InterfaceDown,EEchoesShellAction::InterfaceUp);
            Volume(LOCTEXT("Ambience", "Ambience"),Settings->GetAmbienceVolume(),EEchoesShellAction::AmbienceDown,EEchoesShellAction::AmbienceUp);
        }
        if (!Settings)
        {
            Button(LOCTEXT("Controls", "Controls and key bindings"), EEchoesShellAction::OpenControls);
        }
        if (IsOnlineLocalMenuShellRouteActive())
        {
            View.Status = LOCTEXT("OnlineOptionsContinuity",
                "ONLINE MATCH CONTINUES — local controls are held while this menu is open.");
        }
        Back(); break;
    case EEchoesShellScreen::Gameplay: break;
    }
    if (IsOnlineLocalMenuShellRouteActive() &&
        (View.Screen == EEchoesShellScreen::Confirmation || View.Screen == EEchoesShellScreen::DisplayConfirmation || View.Screen == EEchoesShellScreen::Error))
    {
        const FText Continuity = LOCTEXT("OnlineDialogContinuity", "ONLINE MATCH CONTINUES while this dialog is open.");
        View.Status = View.Status.IsEmpty() ? Continuity
            : FText::Format(LOCTEXT("OnlineDialogStatus", "{0}\n{1}"), Continuity, View.Status);
    }
    if (View.Screen == EEchoesShellScreen::Title || View.Screen == EEchoesShellScreen::Pause)
    {
        const FText ArtNotice = PreReleaseArtNotice();
        if (!ArtNotice.IsEmpty())
        {
            View.Body = View.Body.IsEmpty() ? ArtNotice
                : FText::Format(LOCTEXT("PreReleaseArtBody", "{0}\n\n{1}"), View.Body, ArtNotice);
        }
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
    if (Action == EEchoesShellAction::OpenResourceMonitor) { OpenResourceMonitor(); RefreshShell(); return; }
    if (HandleFeedbackHistoryShellAction(Action)) { RefreshShell(); return; }
    if (Action != EEchoesShellAction::ResetBindings && HandleControlsShellAction(Action, Argument))
    { RefreshShell(); return; }
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
    const auto StartTutorial = [&](uint16 PracticeBit)
    {
        const auto PriorPractice = TutorialPractice;
        if (PracticeBit != 0)
        {
            if (!TutorialPractice.Begin(PracticeBit)) return;
        }
        else TutorialPractice.Reset();
        // An explicit learning attempt opts back in durably, including practice.
        // Otherwise recovery would suppress the session the player just chose.
        const FEchoesPlayerProfile PriorProfile = PlayerProfile;
        PlayerProfile.bOnboardingOffered = true;
        PlayerProfile.bTutorialOptOut = false;
        if (!CommitPlayerProfile())
        {
            PlayerProfile = PriorProfile;
            TutorialPractice = PriorPractice;
            Fail(ShellMessage);
            return;
        }
        const bool bRestartTraining = Bridge->GetOperationMode() == EEchoesOperationMode::TrainingReadiness;
        Bridge->SetTrainingPracticeTarget(PracticeBit);
        if (!Bridge->SelectOperationMode(EEchoesOperationMode::TrainingReadiness, Feedback)) { Fail(Feedback); return; }
        if (!(bRestartTraining ? Bridge->RestartPrototypeScenario() : Bridge->StartPrototypeScenario()))
        {
            Fail(LOCTEXT("TrainingStartFailed", "The readiness check could not start. Return to the title menu and try again.").ToString());
            return;
        }
        // Resume the durable record rather than erasing it: zeroing here meant a
        // player who skipped a step and genuinely earned later ones came back to
        // a tutorial that had forgotten both. ResetTutorialObservation restores
        // both masks from the saved profile.
        bTutorialOperationAuthorized = true;
        ResetTutorialObservation();
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
        const bool bLeavingHelp = PlayerFlow.Current() == EEchoesShellScreen::Help;
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
        else if (bLeavingHelp && TutorialPractice.IsActive())
        {
            TutorialPractice.Reset();
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
    case EEchoesShellAction::Tutorial: StartTutorial(0); break;
    case EEchoesShellAction::Help:
        ShellMessage.Reset();
        PlayerFlow.Push(EEchoesShellScreen::Help);
        break;
    case EEchoesShellAction::PracticeTutorialLesson:
        StartTutorial(static_cast<uint16>(Argument));
        break;
    case EEchoesShellAction::Campaign:
    case EEchoesShellAction::Modes:
        if (Action == EEchoesShellAction::Campaign && !RequireOperationProfile())
        { Fail(ShellMessage); break; }
        if (!PlayerProfile.IsTutorialMasteryComplete() && !PlayerProfile.bTutorialOptOut)
        {
            if (!Confirm(LOCTEXT("SkipTraining", "Skip the tutorial for now? You can return to it from the main menu at any time."))) break;
            const FEchoesPlayerProfile PriorProfile = PlayerProfile;
            PlayerProfile.bOnboardingOffered = true;
            PlayerProfile.bTutorialOptOut = true;
            if (!CommitPlayerProfile()) { PlayerProfile = PriorProfile; Fail(ShellMessage); break; }
        }
        if (!RequireOperationProfile())
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
    case EEchoesShellAction::ResetBindings:
        if (Confirm(LOCTEXT("ResetControlsConfirm", "Restore all default control bindings? Your current bindings will be replaced.")))
            ApplyConfirmedDefaultBindings();
        break;
    case EEchoesShellAction::Options:
        SeedPendingDisplayFromLivePresentation();
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
            // Restore what the player is looking at. Taking the restore point from the
            // stored setting reverts to a presentation the window may never have had,
            // and a borderless value restored over a windowed session leaves the window
            // filling the display while Options reports the smaller resolution.
            {
                FIntPoint LiveResolution = PreviousDisplayResolution;
                EWindowMode::Type LiveMode = PreviousDisplayMode;
                if (GetLiveDisplayPresentation(LiveResolution, LiveMode))
                { PreviousDisplayResolution = LiveResolution; PreviousDisplayMode = LiveMode; }
            }
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
        if (!RequireOperationProfile()) { Fail(ShellMessage); break; }
        if (!(Action == EEchoesShellAction::Load ? Bridge->QuickLoadScenario(Feedback) : Bridge->RecoverInterruptedSession(Recovery, Feedback)))
            Fail(Feedback);
        else
        {
            PlayerFlow.ClearOverlays();
            ResetTutorialObservation();
            bTutorialOperationAuthorized = Bridge->GetOperationMode() == EEchoesOperationMode::TrainingReadiness &&
                !PlayerProfile.bTutorialOptOut;
            if (!RequireOperationProfile())
            {
                PresentMissionBriefing();
                break;
            }
            PlayerFlow.SetVisible(PlayerFlow.BaseScreen(), false);
            Bridge->SetScenarioPaused(false);
            ResetIgnoreMoveInput(); ResetIgnoreLookInput();
            ShellMessage = EchoesCheckpointFeedback::Restored().ToString();
            SetStatusMessage(ShellMessage, 8.0f);
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
    {
        const bool bConcedingCampaign = Bridge &&
            Bridge->GetOperationMode() != EEchoesOperationMode::Skirmish;
        if (!Confirm(bConcedingCampaign
                ? LOCTEXT("ConcedeOperationConfirm",
                    "Concede this operation? It is recorded as a failed mission, exactly as if it had been lost in play.")
                : LOCTEXT("ConcedeConfirm", "Concede this match and record a defeat?")))
        {
            break;
        }
        if (!Bridge->ConcedeOfflineMatch(Feedback)) { Fail(Feedback); break; }
        PlayerFlow.ClearOverlays();
        // A skirmish result is reported here. A campaign operation must NOT be, or it
        // would present the skirmish result screen and skip the mission consequence and
        // the campaign ledger commit. Conceding retires the local Command Core, every
        // mission model fails on that, and the operation's own dispatch then reports it
        // through the authored path on the next evaluation.
        if (!bConcedingCampaign)
        {
            NotifyMatchFinished(Bridge->GetMatchOutcome());
        }
        break;
    }
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

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
bool GEchoesLiveDisplayPresentationInjected = false;
FIntPoint GEchoesLiveDisplayResolutionInjected = FIntPoint(1280, 720);
EWindowMode::Type GEchoesLiveDisplayModeInjected = EWindowMode::Windowed;
}

void AEchoesPlayerController::SetLiveDisplayPresentationForTesting(
    bool bPresent, FIntPoint Resolution, EWindowMode::Type Mode)
{
    GEchoesLiveDisplayPresentationInjected = bPresent;
    GEchoesLiveDisplayResolutionInjected = Resolution;
    GEchoesLiveDisplayModeInjected = Mode;
}
#endif

bool AEchoesPlayerController::GetLiveDisplayPresentation(
    FIntPoint& OutResolution, EWindowMode::Type& OutMode) const
{
#if WITH_DEV_AUTOMATION_TESTS
    if (GEchoesLiveDisplayPresentationInjected)
    {
        OutResolution = GEchoesLiveDisplayResolutionInjected;
        OutMode = GEchoesLiveDisplayModeInjected;
        return true;
    }
#endif
    const UWorld* World = GetWorld();
    if (World == nullptr || World->WorldType == EWorldType::PIE) return false;
    const UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (ViewportClient == nullptr || ViewportClient->Viewport == nullptr) return false;
    const TSharedPtr<SWindow> Window = ViewportClient->GetWindow();
    if (!Window.IsValid()) return false;
    // SWindow, not FViewport::GetWindowMode(): the viewport's copy is only updated
    // when a resize runs, so it still reports the constructor's Windowed default on
    // a game that started borderless and was never resized.
    OutMode = Window->GetWindowMode();
    const FIntPoint ViewportSize = ViewportClient->Viewport->GetSizeXY();
    if (OutMode == EWindowMode::Windowed && ViewportSize.X > 0 && ViewportSize.Y > 0)
    {
        OutResolution = ViewportSize;
        return true;
    }
    // FSceneViewport::ResizeFrame forces a borderless or exclusive-fullscreen window
    // to the whole display rectangle and renders the requested resolution through
    // screen percentage, so the window size there is the monitor, not the preference.
    if (const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
        OutResolution = Settings->GetScreenResolution();
    return true;
}

void AEchoesPlayerController::SeedPendingDisplayFromLivePresentation()
{
    if (const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
    {
        PendingDisplayResolution = Settings->GetScreenResolution();
        PendingDisplayMode = Settings->GetFullscreenMode();
    }
    FIntPoint LiveResolution = PendingDisplayResolution;
    EWindowMode::Type LiveMode = PendingDisplayMode;
    if (GetLiveDisplayPresentation(LiveResolution, LiveMode))
    {
        PendingDisplayResolution = LiveResolution;
        PendingDisplayMode = LiveMode;
    }
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
