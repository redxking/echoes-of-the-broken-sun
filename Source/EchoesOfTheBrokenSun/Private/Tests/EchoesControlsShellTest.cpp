// Author: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesPlayerController.h"
#include "EchoesInputBindingModel.h"
#include "EchoesInputPrompt.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/InputSettings.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesControlsShellTest,
    "Echoes.Runtime.UI.ControlsCaptureRoutes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::EngineFilter)

bool FEchoesControlsShellTest::RunTest(const FString& Parameters)
{
    FEchoesScopedTestSaveEnvironment Storage(*this);
    if (!Storage.IsReady()) return false;
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Wrapper.GetTestWorld();
    auto* Bridge = World->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestTrue(TEXT("Scenario starts"), Bridge && Bridge->StartPrototypeScenario())) return false;
    auto* Controller = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Controls route owner exists"), Controller)) return false;
    Controller->PresentTitleScreen();
    Controller->InitializePlayerProfile();
    Controller->HandleShellAction(EEchoesShellAction::Options);
    Controller->HandleShellAction(EEchoesShellAction::OpenControls);
    const auto View = Controller->BuildShellView();
    TestTrue(TEXT("Options opens actual Controls route"), View.Screen == EEchoesShellScreen::Controls);
    TArray<FEchoesInputBinding> Before;
    FEchoesInputBindingModel::EnumerateLive(*GetDefault<UInputSettings>(), Before);
    TestEqual(TEXT("Every live binding has an editable row plus reset and back"), View.Buttons.Num(), Before.Num() + 2);
    if (!Before.IsEmpty())
    {
        Controller->HandleShellAction(EEchoesShellAction::EditBinding, 0);
        TestTrue(TEXT("Row enters capture"), Controller->IsCapturingControlBinding());
        // A modifier alone must not save or complete a chord.
        Controller->CaptureControlBinding(EKeys::LeftShift, true, false, false, false);
        TestTrue(TEXT("Modifier alone waits for a key"), Controller->IsCapturingControlBinding());
        Controller->CaptureControlBinding(EKeys::Escape, false, false, false, false);
        TestFalse(TEXT("Escape cancels capture"), Controller->IsCapturingControlBinding());
        TestTrue(TEXT("Cancel returns to Controls"), Controller->BuildShellView().Screen == EEchoesShellScreen::Controls);
    }
    Controller->HandleShellAction(EEchoesShellAction::ResetBindings);
    TestTrue(TEXT("Reset requires the existing confirmation route"), Controller->BuildShellView().Screen == EEchoesShellScreen::Confirmation);
    Controller->HandleShellAction(EEchoesShellAction::Cancel);
    TArray<FEchoesInputBinding> After;
    FEchoesInputBindingModel::EnumerateLive(*GetDefault<UInputSettings>(), After);
    TestTrue(TEXT("Cancelling capture and reset preserves every binding"), Before == After);
    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("Back preserves the Options return stack"), Controller->BuildShellView().Screen == EEchoesShellScreen::Options);
    Bridge->StopPrototypeScenario();
    Wrapper.ForwardErrorMessages(this);
    return true;
}
#endif
