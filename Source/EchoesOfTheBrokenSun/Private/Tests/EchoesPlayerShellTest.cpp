#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesPlayerController.h"
#include "EchoesCinematicSubsystem.h"
#include "EchoesPlayerProfile.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesShellWidget.h"
#include "EchoesGameUserSettings.h"
#include "EchoesNarrativeSubsystem.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Components/InputComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationCommon.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

namespace
{
// Exercise the real SaveSettings route inside the launcher's isolated UserDir.
// UE reloads the generated GameUserSettings path during SaveSettings, so changing
// the global filename would not isolate its writes. Preserve both file and cache.
class FScopedShellDisplayConfig final
{
public:
    explicit FScopedShellDisplayConfig(FAutomationTestBase& InTest)
        : Test(InTest), PriorCommandLine(FCommandLine::Get()),
          GameKey(GGameUserSettingsIni), PriorEditorIni(GEditorSettingsIni)
    {
        // UE5.8 known config globals are cache keys ("GameUserSettings"),
        // not filenames. Resolve the branch's actual output path for disk I/O.
        const FConfigBranch* GameBranch = GConfig->FindBranch(TEXT("GameUserSettings"), GameKey);
        GameIni = GameBranch ? GameBranch->IniPath : FString();
        FString UserDir;
        if (GameIni.IsEmpty() || !FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir) ||
            !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(GameIni), UserDir))
        {
            Test.AddError(FString::Printf(TEXT("Display persistence requires GameUserSettings inside the isolated UserDir. key=%s path=%s userDir=%s"),
                *GameKey, *GameIni, *UserDir));
            return;
        }
        // Scalability uses the global editor settings path. Unlike GameUserSettings,
        // that path is not reloaded by SaveSettings and can remain GUID-scoped.
        EditorIni = FPaths::Combine(FEchoesCampaignProgressStore::GetSaveGameDirectory(), TEXT("DisplayEditorSettings.ini"));
        GEditorSettingsIni = EditorIni;
        for (const FString& Path : {GameIni, EditorIni})
        {
            FPreservedConfig& Entry = Preserved.AddDefaulted_GetRef();
            Entry.Path = Path;
            Entry.Key = Path == GameIni ? GameKey : Path;
            Entry.bExisted = IFileManager::Get().FileExists(*Path);
            if (Entry.bExisted && !FFileHelper::LoadFileToArray(Entry.Bytes, *Path))
            {
                Test.AddError(TEXT("Could not preserve the isolated display config."));
                return;
            }
            if (const FConfigFile* Cached = GConfig->FindConfigFile(Entry.Key))
            {
                Entry.Cache = MakeUnique<FConfigFile>(*Cached);
            }
        }
        FCommandLine::Append(TEXT(" -MultiprocessSaveConfig"));
        bReady = true;
    }

    ~FScopedShellDisplayConfig()
    {
        FCommandLine::Set(*PriorCommandLine);
        GEditorSettingsIni = PriorEditorIni;
        if (!bReady) return;
        for (const FPreservedConfig& Entry : Preserved)
        {
            if (Entry.Cache) GConfig->SetFile(Entry.Key, Entry.Cache.Get());
            else GConfig->Remove(Entry.Key);
            const bool bRestored = Entry.bExisted
                ? FFileHelper::SaveArrayToFile(Entry.Bytes, *Entry.Path)
                : IFileManager::Get().Delete(*Entry.Path, false, true);
            Test.TestTrue(TEXT("Isolated display config restored after persistence test"), bRestored);
        }
    }

    bool IsReady() const { return bReady; }
    const FString& GetGameIni() const { return GameIni; }
    const FString& GetGameKey() const { return GameKey; }

private:
    struct FPreservedConfig
    {
        FString Path;
        FString Key;
        TArray<uint8> Bytes;
        TUniquePtr<FConfigFile> Cache;
        bool bExisted = false;
    };
    FAutomationTestBase& Test;
    FString PriorCommandLine;
    FString GameKey;
    FString GameIni;
    FString PriorEditorIni;
    FString EditorIni;
    TArray<FPreservedConfig> Preserved;
    bool bReady = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesPlayerShellTest,
    "Echoes.Runtime.UI.PlayerShellRoutes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FEchoesPlayerShellTest::RunTest(const FString&)
{
    FEchoesScopedTestSaveEnvironment Storage(*this);
    if (!Storage.IsReady()) return false;
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Wrapper.GetTestWorld();
    auto* Bridge = World->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestTrue(TEXT("Scenario starts"), Bridge && Bridge->StartPrototypeScenario())) return false;
    auto* Controller = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Player shell controller exists"), Controller))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->InitInputSystem();
    if (!TestNotNull(TEXT("Player shell controller initializes input"),
                     Controller->PlayerInput.Get()))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->PresentTitleScreen();
    TestTrue(TEXT("Fresh isolated profile initializes"), Controller->InitializePlayerProfile());
    TestTrue(TEXT("Fresh primary action offers tutorial"), Controller->BuildShellView().Buttons[0].Action == EEchoesShellAction::Tutorial);
    // AssetRegister.md requires placeholders to stay visibly labeled in development
    // builds. -EchoesFinalArtPath is the documented opt-out, so honour it here rather
    // than asserting a state the launch arguments may legitimately have suppressed.
    const bool bArtNoticeExpected = !FParse::Param(FCommandLine::Get(), TEXT("EchoesFinalArtPath"));
    TestTrue(TEXT("Title discloses that art is not final"),
        Controller->BuildShellView().Body.ToString().Contains(TEXT("The art and graphics are not finished."))
            == bArtNoticeExpected);
    TestTrue(TEXT("Title keeps its own body alongside the art notice"),
        Controller->BuildShellView().Body.ToString().Contains(TEXT("The sun is broken.")));
    const uint64 InitialChecksum = Bridge->GetSimulation()->StateChecksum();
    Controller->HandleShellAction(EEchoesShellAction::Options);
    TestTrue(TEXT("Options overlays title"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Options && Controller->IsTitleScreenVisible());
    TestFalse(TEXT("Art notice stays on the two screens a player reads, not every shell screen"),
        Controller->BuildShellView().Body.ToString().Contains(TEXT("The art and graphics are not finished.")));
    if (auto* Settings = UEchoesGameUserSettings::Get())
    {
        const FEchoesShellView OptionsView = Controller->BuildShellView();
        const auto OptionIndex = [&OptionsView](EEchoesShellAction Action)
        {
            return OptionsView.Buttons.IndexOfByPredicate([Action](const FEchoesShellButton& Button)
                { return Button.Action == Action; });
        };
        const int32 ControlsIndex = OptionIndex(EEchoesShellAction::OpenControls);
        TestTrue(TEXT("Options keeps accessibility together before the Controls and Camera sections"),
            OptionIndex(EEchoesShellAction::HudScaleDown) == 0 &&
            OptionIndex(EEchoesShellAction::HudScaleUp) == 1 &&
            OptionIndex(EEchoesShellAction::ReducedFlashing) == ControlsIndex - 1 &&
            OptionIndex(EEchoesShellAction::EdgePan) == ControlsIndex + 1);
        const FScopedShellDisplayConfig DisplayConfig(*this);
        if (!DisplayConfig.IsReady()) { Bridge->StopPrototypeScenario(); return false; }
        const FEchoesPlayerProfile Original = Controller->GetPlayerProfile();
        const FIntPoint PriorConfirmedResolution = Settings->GetLastConfirmedScreenResolution();
        const EWindowMode::Type PriorConfirmedMode = Settings->GetLastConfirmedFullscreenMode();
        Controller->HandleShellValue(EEchoesShellAction::HudScaleValue, 1.237f, true);
        TestTrue(TEXT("Continuous HUD scale retains a non-step value"), FMath::IsNearlyEqual(Settings->GetHudScale(), 1.237f));
        Controller->HandleShellValue(EEchoesShellAction::HudScaleValue, -2.f, true);
        TestEqual(TEXT("Continuous HUD scale clamps to80percent"), Settings->GetHudScale(), .8f);
        Controller->HandleShellValue(EEchoesShellAction::HudScaleValue, 2.f, true);
        TestEqual(TEXT("Continuous HUD scale clamps to150percent"), Settings->GetHudScale(), 1.5f);
        Controller->HandleShellAction(EEchoesShellAction::CameraPanUp);
        TestTrue(TEXT("Camera pan changes through shell and profile"), Settings->GetCameraPanSpeedScale() > Original.CameraPanSpeedScale && Controller->GetPlayerProfile().CameraPanSpeedScale == Settings->GetCameraPanSpeedScale());
        Controller->HandleShellAction(EEchoesShellAction::CameraZoomDown);
        TestTrue(TEXT("Camera zoom changes through shell and profile"), Settings->GetCameraZoomScale() < Original.CameraZoomScale);
        const FIntPoint OriginalResolution = Settings->GetScreenResolution();
        const EWindowMode::Type OriginalMode = Settings->GetFullscreenMode();
        Controller->HandleShellAction(EEchoesShellAction::ResolutionNext);
        Controller->HandleShellAction(EEchoesShellAction::WindowMode);
        TestEqual(TEXT("Unapplied display choice leaves active resolution intact"), Settings->GetScreenResolution(), OriginalResolution);
        Controller->HandleShellAction(EEchoesShellAction::ApplyDisplay);
        TestTrue(TEXT("Display apply enters confirmation"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::DisplayConfirmation);
        TestEqual(TEXT("Preview never commits profile display"), Controller->GetPlayerProfile().Resolution, OriginalResolution);
        Controller->HandleShellAction(EEchoesShellAction::Back);
        TestEqual(TEXT("Back restores prior resolution"), Settings->GetScreenResolution(), OriginalResolution);
        TestTrue(TEXT("Back restores mode and Options"), Settings->GetFullscreenMode() == OriginalMode && Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Options);
        Controller->HandleShellAction(EEchoesShellAction::ResolutionNext);
        Controller->HandleShellAction(EEchoesShellAction::WindowMode);
        Controller->HandleShellAction(EEchoesShellAction::ApplyDisplay);
        FEchoesPlayerProfileStore::FailNextCommitForTesting();
        Controller->HandleShellAction(EEchoesShellAction::KeepDisplay);
        TestTrue(TEXT("Failed display commit reports an error and restores active mode"),
            Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Error && Settings->GetScreenResolution() == OriginalResolution);
        Controller->HandleShellAction(EEchoesShellAction::Retry);
        TestTrue(TEXT("Display retry previews the requested choice again"),
            Controller->GetPlayerFlow().Current() == EEchoesShellScreen::DisplayConfirmation && Settings->GetScreenResolution() != OriginalResolution);
        Controller->HandleShellAction(EEchoesShellAction::KeepDisplay);
        TestTrue(TEXT("Kept display persists"), Controller->GetPlayerProfile().Resolution == Settings->GetScreenResolution() && Settings->GetScreenResolution() != OriginalResolution);
        TestEqual(TEXT("Kept live confirmation matches active resolution"), Settings->GetLastConfirmedScreenResolution(), Settings->GetScreenResolution());
        TestEqual(TEXT("Kept live confirmation matches active window mode"), Settings->GetLastConfirmedFullscreenMode(), Settings->GetFullscreenMode());
        TestEqual(TEXT("SaveSettings preserves the isolated generated INI route"), GGameUserSettingsIni, DisplayConfig.GetGameKey());
        FConfigFile SavedDisplayConfig;
        SavedDisplayConfig.Read(DisplayConfig.GetGameIni());
        TestFalse(TEXT("Kept display settings can be reopened from disk"), SavedDisplayConfig.IsEmpty());
        const FString SettingsSection = Settings->GetClass()->GetPathName();
        int32 ConfirmedX = 0, ConfirmedY = 0, ConfirmedMode = -1;
        TestTrue(TEXT("Confirmed display width is serialized"), SavedDisplayConfig.GetInt(*SettingsSection, TEXT("LastUserConfirmedResolutionSizeX"), ConfirmedX));
        TestTrue(TEXT("Confirmed display height is serialized"), SavedDisplayConfig.GetInt(*SettingsSection, TEXT("LastUserConfirmedResolutionSizeY"), ConfirmedY));
        TestTrue(TEXT("Confirmed display mode is serialized"), SavedDisplayConfig.GetInt(*SettingsSection, TEXT("LastConfirmedFullscreenMode"), ConfirmedMode));
        TestEqual(TEXT("Confirmed disk resolution matches kept mode"), FIntPoint(ConfirmedX, ConfirmedY), Settings->GetScreenResolution());
        TestEqual(TEXT("Confirmed disk window mode matches kept mode"), ConfirmedMode, static_cast<int32>(Settings->GetFullscreenMode()));
        const FIntPoint KeptResolution = Settings->GetScreenResolution();
        Controller->HandleShellAction(EEchoesShellAction::ResolutionNext);
        Controller->HandleShellAction(EEchoesShellAction::ApplyDisplay);
        // Exercise the production monotonic deadline, including a frame gap.
        FPlatformProcess::SleepNoStats(15.05f);
        Controller->RefreshShell();
        TestTrue(TEXT("Unconfirmed display automatically reverts after fifteen seconds"),
            Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Options && Settings->GetScreenResolution() == KeptResolution);
        TestEqual(TEXT("Timeout never commits the abandoned display choice"), Controller->GetPlayerProfile().Resolution, KeptResolution);
        Controller->HandleShellAction(EEchoesShellAction::ResolutionNext);
        Controller->HandleShellAction(EEchoesShellAction::ApplyDisplay);
        const FString ConfirmationBody = Controller->BuildShellView().Body.ToString();
        Controller->HandleShellAction(EEchoesShellAction::RevertDisplay);
        TestTrue(TEXT("Display confirmation displays the remaining wall time"),
            ConfirmationBody.Contains(TEXT("reverts automatically in 15 seconds")));
        // SPEC-UI-009.CONFIRM/.TIMEOUT restore "the previous valid mode", and .TIMEOUT
        // additionally requires the reverted mode to remain usable. The stored setting
        // is not that mode whenever the window never adopted it: a -windowed command
        // line, an engine clamp during window creation, or a mode the platform refused
        // all leave GameUserSettings describing a window that does not exist. Reverting
        // to the stored value then puts the window into a presentation the player never
        // chose - observed as a borderless full-display window while Options and the
        // engine both reported 1280x720, with no route back through a greyed-out Apply.
        {
            struct FScopedInjectedPresentation final
            {
                ~FScopedInjectedPresentation()
                { AEchoesPlayerController::SetLiveDisplayPresentationForTesting(false); }
            } InjectedPresentation;
            Settings->SetScreenResolution(FIntPoint(1280, 720));
            Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
            AEchoesPlayerController::SetLiveDisplayPresentationForTesting(
                true, FIntPoint(1280, 720), EWindowMode::Windowed);
            Controller->HandleShellAction(EEchoesShellAction::Back);
            Controller->HandleShellAction(EEchoesShellAction::Options);
            const FEchoesShellView MismatchedView = Controller->BuildShellView();
            const auto FindButton = [&MismatchedView](EEchoesShellAction Action)
            {
                return MismatchedView.Buttons.FindByPredicate(
                    [Action](const FEchoesShellButton& Button) { return Button.Action == Action; });
            };
            const FEchoesShellButton* ModeButton = FindButton(EEchoesShellAction::WindowMode);
            const FEchoesShellButton* ApplyButton = FindButton(EEchoesShellAction::ApplyDisplay);
            TestTrue(TEXT("Options reports the window on screen, not a stored mode the window never adopted"),
                ModeButton != nullptr && ModeButton->Label.ToString().Contains(TEXT("Windowed")));
            TestTrue(TEXT("Apply stays reachable while the stored display settings do not describe the window"),
                ApplyButton != nullptr && ApplyButton->bEnabled);
            Controller->HandleShellAction(EEchoesShellAction::ResolutionNext);
            Controller->HandleShellAction(EEchoesShellAction::ApplyDisplay);
            TestTrue(TEXT("Mismatched display apply still enters confirmation"),
                Controller->GetPlayerFlow().Current() == EEchoesShellScreen::DisplayConfirmation);
            Controller->HandleShellAction(EEchoesShellAction::RevertDisplay);
            TestTrue(TEXT("Revert returns to Options from a mismatched presentation"),
                Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Options);
            TestTrue(TEXT("Revert restores the window mode actually on screen"),
                Settings->GetFullscreenMode() == EWindowMode::Windowed);
            TestEqual(TEXT("Revert restores the resolution actually on screen"),
                Settings->GetScreenResolution(), FIntPoint(1280, 720));
        }
        FString RestoreError; Original.ApplySettings(*Settings, RestoreError);
        Settings->SetScreenResolution(PriorConfirmedResolution);
        Settings->SetFullscreenMode(PriorConfirmedMode);
        Settings->ConfirmVideoMode();
        Settings->SetScreenResolution(OriginalResolution);
        Settings->SetFullscreenMode(OriginalMode);
        if (!FApp::IsUnattended() && World->WorldType != EWorldType::PIE)
            Settings->ApplyResolutionSettings(false);
        TestTrue(TEXT("Option fixture restores original settings"), Controller->CommitPlayerProfile());
    }

    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("Back restores exact title"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Title);
    Controller->HandleShellAction(EEchoesShellAction::Modes);
    TestTrue(TEXT("First-run opt out needs confirmation"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Confirmation);
    Controller->HandleShellAction(EEchoesShellAction::Cancel);
    TestFalse(TEXT("Cancel cannot grant mastery or opt out"), Controller->GetPlayerProfile().IsTutorialMasteryComplete() || Controller->GetPlayerProfile().bTutorialOptOut);
    TestEqual(TEXT("Menu navigation never advances simulation"), Bridge->GetSimulation()->StateChecksum(), InitialChecksum);
    Controller->HandleShellAction(EEchoesShellAction::Modes);
    FEchoesPlayerProfileStore::FailNextCommitForTesting();
    Controller->HandleShellAction(EEchoesShellAction::Confirm);
    TestTrue(TEXT("Failed opt-out write offers retry"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Error);
    TestFalse(TEXT("Failed write cannot retain opt-out in memory"), Controller->GetPlayerProfile().bTutorialOptOut);
    Controller->HandleShellAction(EEchoesShellAction::Retry);
    TestTrue(TEXT("Confirmed opt out opens skirmish setup without mastery"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Modes);
    TestTrue(TEXT("Opt out persists separately from mastery"), Controller->GetPlayerProfile().bTutorialOptOut && !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    FEchoesPlayerProfile Reloaded;
    bool bExists = false; FString Feedback;
    TestTrue(TEXT("Profile survives fresh load"), FEchoesPlayerProfileStore::LoadWithBackup(FEchoesPlayerProfileStore::GetDefaultPath(), Reloaded, bExists, Feedback) && bExists && Reloaded.bTutorialOptOut);
    // Approved optional onboarding must permit an actual unmastered deployment.
    Controller->HandleShellAction(EEchoesShellAction::Primary);
    if (!TestTrue(TEXT("Unmastered setup reaches a ready briefing"),
        Controller->IsMissionBriefingVisible() && Bridge->IsScenarioPaused())) return false;
    TestTrue(TEXT("Unmastered briefing offers enabled deployment"),
        Controller->BuildShellView().Buttons.ContainsByPredicate([](const FEchoesShellButton& Button)
            { return Button.Action == EEchoesShellAction::Primary && Button.bEnabled; }));
    Controller->HandleShellAction(EEchoesShellAction::Primary);
    TestTrue(TEXT("Opt-out deploys full skirmish without fabricating mastery"),
        !Bridge->IsScenarioPaused() && !Controller->IsMissionBriefingVisible() &&
        !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    Controller->TogglePauseMenu();
    Controller->HandleShellAction(EEchoesShellAction::Resume);
    TestFalse(TEXT("Unmastered player resumes normally"), Bridge->IsScenarioPaused());
    Controller->PresentTitleScreen();
    TestTrue(TEXT("Title still recommends tutorial as the primary action"),
        Controller->BuildShellView().Buttons[0].Action == EEchoesShellAction::Tutorial);
    TestTrue(TEXT("Title retains an enabled tutorial route"),
        Controller->BuildShellView().Buttons.ContainsByPredicate([](const FEchoesShellButton& Button)
            { return Button.Action == EEchoesShellAction::Tutorial && Button.bEnabled; }));
    auto* OwnershipController = World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(TEXT("Ownership boundary fixture controller exists"), OwnershipController))
    {
        TestTrue(TEXT("Non-player runtime authority has no profile gate"),
            OwnershipController->RequireOperationProfile());
        OwnershipController->Player = NewObject<ULocalPlayer>(GEngine);
        TestTrue(TEXT("Attaching a local player loads the valid profile without requiring mastery"),
            OwnershipController->RequireOperationProfile());
        TestTrue(TEXT("M01 checkpoint preflight is permitted"),
            OwnershipController->RequireOperationProfile());
        TestTrue(TEXT("Campaign access also requires only the valid profile"),
            OwnershipController->RequireOperationProfile());
        OwnershipController->Player = nullptr;
        OwnershipController->Destroy();
    }
    TestTrue(TEXT("Full-operation recovery fixture writes a valid checkpoint"), Bridge->QuickSaveScenario(Feedback));
    FEchoesRecoveryCandidate FullRecovery;
    TestTrue(TEXT("Recovery scan admits the full-operation fixture before the player gate"),
        Bridge->CheckInterruptedSessionRecovery(FullRecovery, Feedback) &&
        FullRecovery.OperationMode == EEchoesOperationMode::Skirmish);
    const uint64 BeforeRecovery = Bridge->GetSimulation()->StateChecksum();
    TArray<uint8> BeforeRecoveryBytes, AfterRecoveryBytes;
    FFileHelper::LoadFileToArray(BeforeRecoveryBytes, *Bridge->GetActiveQuickSavePath());
    Controller->HandleShellAction(EEchoesShellAction::SaveLoad);
    Controller->HandleShellAction(EEchoesShellAction::Recover);
    Controller->HandleShellAction(EEchoesShellAction::Confirm);
    TestTrue(TEXT("Unmastered full-operation recovery restores playable skirmish"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Gameplay && !Bridge->IsScenarioPaused() &&
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish && !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    TestEqual(TEXT("Recovery restores the saved simulation checksum"), Bridge->GetSimulation()->StateChecksum(), BeforeRecovery);
    FFileHelper::LoadFileToArray(AfterRecoveryBytes, *Bridge->GetActiveQuickSavePath());
    TestTrue(TEXT("Recovery preserves checkpoint bytes"), BeforeRecoveryBytes == AfterRecoveryBytes && !BeforeRecoveryBytes.IsEmpty());
    // The file API can report only whole-second modification times. Establish
    // explicit ordering between independent fixtures instead of assuming the
    // subsequent learning checkpoint gets a distinct timestamp in a fast run.
    const FDateTime PriorFixtureTime = FDateTime::UtcNow() - FTimespan::FromMinutes(1);
    IFileManager::Get().SetTimeStamp(*FullRecovery.SourcePath, PriorFixtureTime);
    TestTrue(TEXT("Earlier full-operation fixture has an explicitly older timestamp"),
        IFileManager::Get().GetTimeStamp(*FullRecovery.SourcePath) < FDateTime::UtcNow() - FTimespan::FromSeconds(30));
    Controller->PresentTitleScreen();
    Controller->HandleShellAction(EEchoesShellAction::Tutorial);
    Controller->HandleShellAction(EEchoesShellAction::Primary);
    auto* Opening = World->GetSubsystem<UEchoesCinematicSubsystem>();
    TestTrue(TEXT("M01 opening holds deployment and does not grant mastery"),
        Opening && Opening->IsSequenceActive(EEchoesCinematicSequence::M01Opening) &&
        Bridge->IsScenarioPaused() && !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    FInputKeyEventArgs SkipOpening;
    SkipOpening.Key = EKeys::Escape;
    SkipOpening.Event = IE_Pressed;
    Controller->InputKey(SkipOpening);
    Controller->PlayerTick(0.0f);
    TestTrue(TEXT("Escape completes opening before tutorial deployment"),
        Opening && Opening->HasSequenceCompleted(EEchoesCinematicSequence::M01Opening));
    TestTrue(TEXT("Explicit tutorial route remains playable without granting mastery"),
        Bridge->GetOperationMode() == EEchoesOperationMode::TrainingReadiness &&
        !Bridge->IsScenarioPaused() && !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    Controller->OpenTutorialSkipModal();
    Controller->EndAllTutorials();
    TestTrue(TEXT("End all persists opt-out and stops guidance before checkpoint"),
        Controller->GetPlayerProfile().bTutorialOptOut && !Controller->IsTutorialOperationAuthorized() &&
        Controller->GetTutorialInstruction().IsEmpty() && !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    TestTrue(TEXT("Learning operation checkpoint writes"), Bridge->QuickSaveScenario(Feedback));
    FEchoesRecoveryCandidate LearningRecovery;
    TestTrue(TEXT("Newest recovery candidate is the learning operation"),
        Bridge->CheckInterruptedSessionRecovery(LearningRecovery, Feedback) &&
        LearningRecovery.OperationMode == EEchoesOperationMode::TrainingReadiness);
    Controller->Destroy();
    Controller = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Learning recovery uses a fresh controller"), Controller))
    {
        Bridge->StopPrototypeScenario();
        return false;
    }
    Controller->InitInputSystem();
    Controller->PresentTitleScreen();
    if (!TestTrue(TEXT("Learning recovery reloads the unmastered profile"), Controller->InitializePlayerProfile()))
    {
        Bridge->StopPrototypeScenario();
        return false;
    }
    Controller->HandleShellAction(EEchoesShellAction::SaveLoad);
    Controller->HandleShellAction(EEchoesShellAction::Recover);
    Controller->HandleShellAction(EEchoesShellAction::Confirm);
    TestTrue(TEXT("Successful training recovery admits learning without granting mastery"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Gameplay &&
        Bridge->GetOperationMode() == EEchoesOperationMode::TrainingReadiness &&
        !Bridge->IsScenarioPaused() && !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    TestTrue(TEXT("Fresh-controller recovery honors ended tutorials"),
        !Controller->IsTutorialOperationAuthorized() && Controller->GetTutorialInstruction().IsEmpty() &&
        !Controller->BuildFieldHudView().bTutorialActive);
    const uint64 BeforeHotkeyRestore = Bridge->GetScenarioAuthorityGeneration();
    bool bQuickLoadBound = false;
    if (Controller->InputComponent)
    {
        for (int32 Index = 0; Index < Controller->InputComponent->GetNumActionBindings(); ++Index)
        {
            const auto& Binding = Controller->InputComponent->GetActionBinding(Index);
            if (Binding.GetActionName() == TEXT("QuickLoadScenario") && Binding.KeyEvent == IE_Pressed)
            {
                Binding.ActionDelegate.Execute(EKeys::L);
                bQuickLoadBound = true;
                break;
            }
        }
    }
    TestTrue(TEXT("Quick-load input binding restores the training authority"), bQuickLoadBound &&
        Bridge->GetScenarioAuthorityGeneration() > BeforeHotkeyRestore &&
        Bridge->GetOperationMode() == EEchoesOperationMode::TrainingReadiness &&
        !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    TestFalse(TEXT("Quick-load cannot reenable opted-out tutorial guidance"), Controller->IsTutorialOperationAuthorized());
    TestEqual(TEXT("Quick-load reports restored gameplay instead of stale pause status"),
        Controller->GetStatusMessage(), FString(TEXT("Checkpoint restored. Ready for your command.")));
    Controller->PresentTitleScreen(); // Replay must use an action actually offered by the menu.
    FEchoesPlayerProfileStore::FailNextCommitForTesting();
    Controller->HandleShellAction(EEchoesShellAction::Tutorial);
    TestTrue(TEXT("Failed explicit opt-in retains opt-out and offers retry"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Error &&
        Controller->GetPlayerProfile().bTutorialOptOut && !Controller->IsTutorialOperationAuthorized());
    Controller->HandleShellAction(EEchoesShellAction::Retry);
    TestTrue(TEXT("Explicit tutorial replay durably opts in without mastery"),
        Controller->IsTutorialOperationAuthorized() && !Controller->GetPlayerProfile().bTutorialOptOut &&
        FEchoesPlayerProfileStore::LoadWithBackup(FEchoesPlayerProfileStore::GetDefaultPath(), Reloaded, bExists, Feedback) &&
        bExists && !Reloaded.bTutorialOptOut && !Reloaded.IsTutorialMasteryComplete());
    if (!TestTrue(TEXT("Explicit replay writes a real training checkpoint"), Bridge->QuickSaveScenario(Feedback))) return false;
    Controller->PresentTitleScreen();
    Controller->HandleShellAction(EEchoesShellAction::SaveLoad);
    if (!TestTrue(TEXT("Title recovery offers the restore action"),
        Controller->BuildShellView().Buttons.ContainsByPredicate([](const FEchoesShellButton& Button)
            { return Button.Action == EEchoesShellAction::Recover && Button.bEnabled; }))) return false;
    Controller->HandleShellAction(EEchoesShellAction::Recover);
    if (!TestTrue(TEXT("Explicit replay recovery reaches confirmation"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Confirmation)) return false;
    Controller->HandleShellAction(EEchoesShellAction::Confirm);
    TestEqual(TEXT("Menu recovery replaces stale field-menu status"),
        Controller->GetStatusMessage(), FString(TEXT("Checkpoint restored. Ready for your command.")));
    TestTrue(TEXT("Explicit replay recovery retains guidance and no mastery"),
        Bridge->GetOperationMode() == EEchoesOperationMode::TrainingReadiness &&
        Controller->IsTutorialOperationAuthorized() && !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    // Campaign creates its own checkpoint; run it after the newest-training
    // recovery checks so independent fixture saves cannot race in one second.
    Controller->PresentTitleScreen();
    Controller->HandleShellAction(EEchoesShellAction::Campaign);
    if (!TestTrue(TEXT("Leaving a restarted tutorial offers optional-training confirmation"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Confirmation)) return false;
    Controller->HandleShellAction(EEchoesShellAction::Confirm);
    TestTrue(TEXT("Optional tutorial permits campaign briefing without mastery"),
        Controller->IsMissionBriefingVisible() && Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue &&
        !Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    Controller->PresentTitleScreen();
    Controller->HandleShellAction(EEchoesShellAction::SaveLoad);
    Controller->HandleShellAction(EEchoesShellAction::SelectSlot, 2);
    TestEqual(TEXT("Runtime slot 2 selected"), Bridge->GetActiveJourneySlot(), 2);
    TestTrue(TEXT("Selected slot persists"), FEchoesPlayerProfileStore::LoadWithBackup(FEchoesPlayerProfileStore::GetDefaultPath(), Reloaded, bExists, Feedback) && Reloaded.ActiveJourneySlot == 2);
    TestTrue(TEXT("Slot2 campaign selected"), Bridge->SelectOperationMode(EEchoesOperationMode::CampaignPrologue, Feedback));
    const FString Slot2Checkpoint = Bridge->GetActiveQuickSavePath();
    TestTrue(TEXT("Slot2 checkpoint writes"), Bridge->QuickSaveScenario(Feedback));
    FEchoesRecoveryCandidate Slot2Recovery;
    TestTrue(TEXT("Slot2 recovery candidate captured"), Bridge->CheckInterruptedSessionRecovery(Slot2Recovery, Feedback));
    Controller->HandleShellAction(EEchoesShellAction::SelectSlot, 3);
    TestEqual(TEXT("Runtime slot3 selected"), Bridge->GetActiveJourneySlot(), 3);
    TestNotEqual(TEXT("Checkpoints are slot isolated"), Bridge->GetActiveQuickSavePath(), Slot2Checkpoint);
    TestFalse(TEXT("Slot3 cannot load slot2 checkpoint"), Bridge->QuickLoadScenario(Feedback));
    TestFalse(TEXT("Stale other-slot recovery rejected"), Bridge->RecoverInterruptedSession(Slot2Recovery, Feedback));
    TestFalse(TEXT("Stale other-slot dismissal rejected"), Bridge->DismissInterruptedSession(Slot2Recovery, Feedback));
    TArray<uint8> Slot2Before; FFileHelper::LoadFileToArray(Slot2Before, *Slot2Checkpoint);
    TestTrue(TEXT("Slot3 independent checkpoint writes"), Bridge->QuickSaveScenario(Feedback));
    TArray<uint8> Slot2After; FFileHelper::LoadFileToArray(Slot2After, *Slot2Checkpoint);
    TestTrue(TEXT("Other-slot writes preserve exact slot2 bytes"), Slot2Before == Slot2After);
    Controller->HandleShellAction(EEchoesShellAction::SelectSlot, 1);
    TestEqual(TEXT("Legacy slot returns without copying"), Bridge->GetActiveJourneySlot(), 1);
    const auto BeforeInvalid = Bridge->GetSimulation()->StateChecksum();
    TestFalse(TEXT("Invalid slot rejected"), Bridge->SelectJourneySlot(4, Feedback));
    TestEqual(TEXT("Invalid slot leaves simulation untouched"), Bridge->GetSimulation()->StateChecksum(), BeforeInvalid);
    TestEqual(TEXT("Invalid slot leaves active journey untouched"), Bridge->GetActiveJourneySlot(), 1);
    // A corrupt different journey cannot replace the active ledger or scenario.
    const TArray<uint8> Corrupt = {1,2,3};
    FFileHelper::SaveArrayToFile(Corrupt, *UEchoesSimulationSubsystem::GetJourneySlotPath(2));
    TestFalse(TEXT("Corrupt slot refused without fallback fabrication"), Bridge->SelectJourneySlot(2, Feedback));
    TestEqual(TEXT("Corrupt slot preserves active journey"), Bridge->GetActiveJourneySlot(), 1);
    // Separate fixture stage: seeded curriculum and readiness facts test
    // profile admission and persistence, not completion of the P3 tutorial.
    FEchoesPlayerProfile VerifiedProfile = Controller->GetPlayerProfile();
    VerifiedProfile.TutorialVerifiedMask = FEchoesPlayerProfile::AllTutorialLessonsMask;
    VerifiedProfile.bReadinessOperationVerified = true;
    TestTrue(TEXT("Verified-profile fixture persists before controller restart"),
        FEchoesPlayerProfileStore::SaveAtomic(FEchoesPlayerProfileStore::GetDefaultPath(), VerifiedProfile, Feedback));
    Controller->Destroy();
    Controller = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Restarted profile controller exists"), Controller)) return false;
    Controller->InitInputSystem();
    if (!TestNotNull(TEXT("Restarted profile controller initializes input"), Controller->PlayerInput.Get()))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Restarted controller loads verified tutorial facts"),
        Controller->InitializePlayerProfile() && Controller->GetPlayerProfile().IsTutorialMasteryComplete());
    Controller->PresentTitleScreen();
    Controller->HandleShellAction(EEchoesShellAction::Help);
    const FEchoesShellView Help = Controller->BuildShellView();
    TestTrue(TEXT("Help presents exactly ten authored lesson rows"),
        Help.Screen == EEchoesShellScreen::Help &&
        Help.Buttons.Num() == 11);
    TestEqual(TEXT("Only the five implemented lessons are replayable"),
        Help.Buttons.FilterByPredicate([](const FEchoesShellButton& Button)
        {
            return Button.Action == EEchoesShellAction::PracticeTutorialLesson &&
                Button.bEnabled;
        }).Num(), 5);
    TestEqual(TEXT("Later lessons remain visibly unavailable"),
        Help.Buttons.FilterByPredicate([](const FEchoesShellButton& Button)
        {
            return Button.Action == EEchoesShellAction::PracticeTutorialLesson &&
                !Button.bEnabled && Button.Label.ToString().Contains(TEXT("unavailable"));
        }).Num(), 5);
    const uint16 DurableBeforePractice =
        Controller->GetPlayerProfile().TutorialVerifiedMask;
    const bool bReadinessBeforePractice =
        Controller->GetPlayerProfile().bReadinessOperationVerified;
    Controller->HandleShellAction(
        EEchoesShellAction::PracticeTutorialLesson, 2);
    TestTrue(TEXT("A mastered Roster lesson opens as a separate practice target"),
        Controller->GetTutorialPracticeTargetBit() == 2 &&
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Briefing);
    FEchoesPlayerProfile PracticeReload;
    TestTrue(TEXT("Choosing practice does not rewrite durable profile facts"),
        Controller->GetPlayerProfile().TutorialVerifiedMask == DurableBeforePractice &&
        Controller->GetPlayerProfile().bReadinessOperationVerified == bReadinessBeforePractice &&
        FEchoesPlayerProfileStore::LoadWithBackup(
            FEchoesPlayerProfileStore::GetDefaultPath(), PracticeReload,
            bExists, Feedback) &&
        PracticeReload.TutorialVerifiedMask == DurableBeforePractice &&
        PracticeReload.bReadinessOperationVerified == bReadinessBeforePractice);
    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("Leaving practice returns to title and clears the transient target"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Title &&
        Controller->GetTutorialPracticeTargetBit() == 0);
    Controller->HandleShellAction(EEchoesShellAction::Modes);
    Controller->HandleShellAction(EEchoesShellAction::Primary);
    Controller->HandleShellAction(EEchoesShellAction::Primary);
    Bridge->Tick(.2f);
    Controller->TogglePauseMenu();
    TestTrue(TEXT("Pause owns single base surface"), Controller->IsPauseMenuVisible() && !Controller->IsTitleScreenVisible() && !Controller->IsMissionBriefingVisible());
    TestTrue(TEXT("Pause discloses that art is not final"),
        Controller->BuildShellView().Body.ToString().Contains(TEXT("The art and graphics are not finished."))
            == bArtNoticeExpected);
    const auto* Narrative = World->GetGameInstance()
        ? World->GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
        : nullptr;
    TestTrue(TEXT("Normal field pause freezes subtitle and voice playback"),
        Narrative && Narrative->IsSubtitlePlaybackPaused());
    const uint64 PauseTick = Bridge->GetSimulation()->CurrentTick();
    Bridge->Tick(1.0f);
    TestEqual(TEXT("Pause freezes authoritative clock"), Bridge->GetSimulation()->CurrentTick(), PauseTick);
    Controller->HandleShellAction(EEchoesShellAction::Restart);
    Controller->HandleShellAction(EEchoesShellAction::Cancel);
    TestTrue(TEXT("Cancel restart returns to pause"), Controller->IsPauseMenuVisible() && Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Pause);
    Controller->HandleShellAction(EEchoesShellAction::Concede);
    TestTrue(TEXT("Concede requires confirmation"), Bridge->GetMatchOutcome() == echoes::sim::MatchOutcome::Ongoing);
    Controller->HandleShellAction(EEchoesShellAction::Confirm);
    TestTrue(TEXT("Confirmed concession reaches authoritative defeat/results"), Bridge->GetMatchOutcome() == echoes::sim::MatchOutcome::Player1Victory && Controller->IsMatchResultVisible());
    // This is a controller route check. It does not stand in for physical input qualification.
    const double ArchiveDeadline = FPlatformTime::Seconds() + 15.0;
    while (Bridge->GetReplayArchiveState() == EEchoesReplayArchiveState::Pending && FPlatformTime::Seconds() < ArchiveDeadline)
    {
        FPlatformProcess::SleepNoStats(.001f);
        Bridge->Tick(0.f);
    }
    if (TestTrue(TEXT("This result has its own committed replay"), Bridge->GetReplayArchiveState() == EEchoesReplayArchiveState::Succeeded))
    {
        const uint64 ResultChecksum = Bridge->GetSimulation()->StateChecksum();
        const uint64 ResultTick = Bridge->GetSimulation()->CurrentTick();
        const bool bResultPaused = Bridge->IsScenarioPaused();
        Controller->HandleShellAction(EEchoesShellAction::ViewReplay);
        if (TestTrue(TEXT("Current result opens detached replay transport"), Bridge->IsReplayPlaybackActive() &&
            Controller->GetPlayerFlow().Current() == EEchoesShellScreen::ReplayTransport))
        {
            Bridge->FailNextReplayPresentationSyncForTesting();
            Controller->HandleShellAction(EEchoesShellAction::ReplayPerspectiveNext);
            const auto FailedPerspective = Bridge->GetReplayPlaybackState().Perspective;
            TestTrue(TEXT("Perspective presentation failure offers retry"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Error);
            Bridge->Tick(0.f); // Recovery may happen before the player clicks Retry.
            Controller->HandleShellAction(EEchoesShellAction::Retry);
            TestTrue(TEXT("Perspective retry returns to transport at the exact requested seat"),
                Controller->GetPlayerFlow().Current() == EEchoesShellScreen::ReplayTransport && Bridge->GetReplayPlaybackState().Perspective == FailedPerspective);
            if (Bridge->GetReplayPlaybackState().FinalTick > 0)
            {
                Bridge->FailNextReplayPresentationSyncForTesting();
                Controller->HandleShellAction(EEchoesShellAction::ReplayStep);
                const uint64 FailedStepTick = Bridge->GetReplayPlaybackState().CurrentTick;
                TestTrue(TEXT("Step presentation failure offers retry"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Error);
                Bridge->Tick(0.f);
                Controller->HandleShellAction(EEchoesShellAction::Retry);
                TestEqual(TEXT("Step retry cannot advance twice after automatic repair"), Bridge->GetReplayPlaybackState().CurrentTick, FailedStepTick);
                Bridge->FailNextReplayPresentationSyncForTesting();
                Controller->HandleShellValue(EEchoesShellAction::ReplaySeek, 1.f, true);
                const uint64 FailedSeekTick = Bridge->GetReplayPlaybackState().CurrentTick;
                TestTrue(TEXT("Seek presentation failure offers retry"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Error);
                Bridge->Tick(0.f);
                Controller->HandleShellAction(EEchoesShellAction::Retry);
                TestEqual(TEXT("Seek retry retains the exact chosen time"), Bridge->GetReplayPlaybackState().CurrentTick, FailedSeekTick);
            }
            for (const FKey& Key : {EKeys::Pause, EKeys::P, EKeys::M, EKeys::R, EKeys::One, EKeys::RightMouseButton})
                TestTrue(TEXT("Replay consumes a live gameplay key"), Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f)));
            Controller->RestartScenario();
            Controller->TogglePauseMenu();
            TestTrue(TEXT("Live actions leave replay transport in place"), Controller->GetPlayerFlow().Current() == EEchoesShellScreen::ReplayTransport);
            TestTrue(TEXT("Replay cannot select live entities"), Controller->GetSelectedEntityIds().IsEmpty());
            TestEqual(TEXT("Replay keys preserve live checksum"), Bridge->GetSimulation()->StateChecksum(), ResultChecksum);
            TestEqual(TEXT("Replay keys preserve live tick"), Bridge->GetSimulation()->CurrentTick(), ResultTick);
            TestEqual(TEXT("Replay keys preserve live pause"), Bridge->IsScenarioPaused(), bResultPaused);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Escape, IE_Pressed, 1.f));
            TestTrue(TEXT("Escape ends playback and returns to the same result"), !Bridge->IsReplayPlaybackActive() && Controller->IsMatchResultVisible());
            TestEqual(TEXT("Replay exit preserves result checksum"), Bridge->GetSimulation()->StateChecksum(), ResultChecksum);
            const auto Dossier = Controller->BuildShellView();
            TestTrue(TEXT("Concession has truthful result copy"), Dossier.Body.ToString().Contains(TEXT("concession")));
            TestEqual(TEXT("Result dossier exposes five recorded metric charts"), Dossier.Charts.Num(), 5);
        }
        else AddError(Controller->BuildShellView().Status.ToString());
    }
    else AddError(Bridge->GetReplayArchiveError());
    Controller->Destroy();
    const FString ProfilePath = FEchoesPlayerProfileStore::GetDefaultPath();
    FEchoesPlayerProfile DamagedJourneyProfile = Reloaded;
    DamagedJourneyProfile.ActiveJourneySlot = 2;
    TestTrue(TEXT("Valid profile can reference a now-corrupt journey"), FEchoesPlayerProfileStore::SaveAtomic(ProfilePath, DamagedJourneyProfile, Feedback));
    auto* SlotRecoveryController = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Slot recovery controller exists"),
                     SlotRecoveryController))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    SlotRecoveryController->InitInputSystem();
    if (!TestNotNull(TEXT("Slot recovery controller initializes input"),
                     SlotRecoveryController->PlayerInput.Get()))
    {
        SlotRecoveryController->Destroy();
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    SlotRecoveryController->PresentTitleScreen();
    TestFalse(TEXT("Corrupt selected journey offers an error"), SlotRecoveryController->InitializePlayerProfile());
    SlotRecoveryController->HandleShellAction(EEchoesShellAction::Back);
    SlotRecoveryController->HandleShellAction(EEchoesShellAction::SaveLoad);
    SlotRecoveryController->HandleShellAction(EEchoesShellAction::SelectSlot, 3);
    TestEqual(TEXT("Damaged journey cannot block selecting another slot"), Bridge->GetActiveJourneySlot(), 3);
    TestTrue(TEXT("Independent profile remains writable"), FEchoesPlayerProfileStore::LoadWithBackup(ProfilePath, Reloaded, bExists, Feedback) && Reloaded.ActiveJourneySlot == 3);
    SlotRecoveryController->Destroy();
    FFileHelper::SaveArrayToFile(Corrupt, *ProfilePath);
    FFileHelper::SaveArrayToFile(Corrupt, *(ProfilePath + TEXT(".bak")));
    auto* RecoveryController = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Profile recovery controller exists"),
                     RecoveryController))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    RecoveryController->InitInputSystem();
    if (!TestNotNull(TEXT("Profile recovery controller initializes input"),
                     RecoveryController->PlayerInput.Get()))
    {
        RecoveryController->Destroy();
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    RecoveryController->PresentTitleScreen();
    TestFalse(TEXT("Two corrupt generations show profile error"), RecoveryController->InitializePlayerProfile());
    TestFalse(TEXT("Optional onboarding never bypasses a corrupt profile"), RecoveryController->RequireOperationProfile());
    RecoveryController->HandleShellAction(EEchoesShellAction::ResetProfile);
    RecoveryController->HandleShellAction(EEchoesShellAction::Cancel);
    TArray<uint8> StillCorrupt; FFileHelper::LoadFileToArray(StillCorrupt, *ProfilePath);
    TestTrue(TEXT("Cancelled reset preserves invalid primary bytes"), StillCorrupt == Corrupt);
    RecoveryController->HandleShellAction(EEchoesShellAction::ResetProfile);
    RecoveryController->HandleShellAction(EEchoesShellAction::Confirm);
    TestTrue(TEXT("Confirmed recovery creates usable profile"), RecoveryController->GetPlayerFlow().Current() == EEchoesShellScreen::Title && RecoveryController->InitializePlayerProfile());
    TestTrue(TEXT("Reset profile remains fresh without fabricated mastery"), !RecoveryController->GetPlayerProfile().IsTutorialMasteryComplete() && !RecoveryController->GetPlayerProfile().bTutorialOptOut);
    RecoveryController->Destroy(); Bridge->StopPrototypeScenario(); Wrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}
#endif
