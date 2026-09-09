// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesFieldHudView.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
bool HasOnlineAction(
    const FEchoesFieldHudView& View,
    EEchoesFieldHudAction Action)
{
    return View.Online.Controls.ContainsByPredicate(
        [Action](const FEchoesFieldHudControl& Control)
        {
            return Control.Action == Action && Control.bEnabled;
        });
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesOnlineLocalMenuRouteTest,
    "Echoes.Runtime.UI.OnlineLocalMenuShellRoutes",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesOnlineLocalMenuRouteTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the online local-menu test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    // The profile must initialize before the scenario runs. SelectJourneySlot
    // refuses while a scenario is ready and unpaused, and this fixture keeps it
    // running on purpose, so a lazy initialization during the first shell action
    // would push an Error screen into the middle of the route under test.
    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Online local-menu route owner exists"), Controller))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // The shell initializes the player profile lazily on its first action, and a
    // failure there pushes an Error screen that would consume the Back this test
    // is measuring. Establish it up front, the way the passing shell tests do.
    if (!TestTrue(TEXT("Fresh isolated profile initializes"),
            Controller->InitializePlayerProfile()))
    {
        Controller->Destroy();
        return false;
    }

    if (!TestTrue(TEXT("Scenario starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }


    // This fixture exercises presentation state only. It deliberately keeps
    // the bridge's scenario running to verify the online menu does not reuse
    // the single-player pause route.
    Bridge->SetNetworkHumanOpponent(true);
    Controller->bNetworkCompatibilityAccepted = true;
    Controller->bNetworkMatchStarted = true;
    Controller->NetworkSeat = UEchoesSimulationSubsystem::LocalPlayerId;
    Controller->bOnlineLocalMenuVisible = true;

    const FEchoesFieldHudView OnlineView = Controller->BuildFieldHudView();
    TestTrue(TEXT("Online field menu presents every local utility route"),
        OnlineView.Surface == EEchoesFieldHudSurface::OnlineLocalMenu &&
        HasOnlineAction(OnlineView, EEchoesFieldHudAction::OnlineResume) &&
        HasOnlineAction(OnlineView, EEchoesFieldHudAction::OnlineOptions) &&
        HasOnlineAction(OnlineView, EEchoesFieldHudAction::OnlineControls) &&
        HasOnlineAction(OnlineView, EEchoesFieldHudAction::OnlineCommandHistory) &&
        HasOnlineAction(OnlineView, EEchoesFieldHudAction::OnlineLeave));
    TestTrue(TEXT("Online field menu says the match continues"),
        OnlineView.Online.State.ToString().Contains(TEXT("MATCH CONTINUES")));
    TestFalse(TEXT("Opening the field menu does not pause the scenario"),
        Bridge->IsScenarioPaused());

    Controller->HandleFieldHudAction(EEchoesFieldHudAction::OnlineOptions);
    FEchoesShellView View = Controller->BuildShellView();
    TestTrue(TEXT("Options opens from the online field menu"),
        View.Screen == EEchoesShellScreen::Options &&
        View.Status.ToString().Contains(TEXT("ONLINE MATCH CONTINUES")));
    TestFalse(TEXT("Options leaves online simulation running"),
        Bridge->IsScenarioPaused());
    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("Options Back returns to the online local menu"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Gameplay &&
        Controller->IsOnlineLocalMenuVisible());

    Controller->HandleFieldHudAction(EEchoesFieldHudAction::OnlineControls);
    View = Controller->BuildShellView();
    TestTrue(TEXT("Controls opens from the online field menu"),
        View.Screen == EEchoesShellScreen::Controls &&
        View.Status.ToString().Contains(TEXT("ONLINE MATCH CONTINUES")));
    Controller->HandleShellAction(EEchoesShellAction::EditBinding, 0);
    TestTrue(TEXT("Online binding capture retains match-continuity warning"),
        Controller->BuildShellView().Screen == EEchoesShellScreen::ControlCapture &&
        Controller->BuildShellView().Status.ToString().Contains(TEXT("ONLINE MATCH CONTINUES")));
    Controller->HandleShellAction(EEchoesShellAction::CancelBinding);
    Controller->HandleShellAction(EEchoesShellAction::ResetBindings);
    TestTrue(TEXT("Online reset confirmation retains match-continuity warning"),
        Controller->BuildShellView().Screen == EEchoesShellScreen::Confirmation &&
        Controller->BuildShellView().Status.ToString().Contains(TEXT("ONLINE MATCH CONTINUES")));
    Controller->HandleShellAction(EEchoesShellAction::Cancel);
    TestFalse(TEXT("Nested online control dialogs never pause the scenario"), Bridge->IsScenarioPaused());
    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("Controls Back returns to the online local menu"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Gameplay &&
        Controller->IsOnlineLocalMenuVisible());

    Controller->HandleFieldHudAction(EEchoesFieldHudAction::OnlineCommandHistory);
    View = Controller->BuildShellView();
    TestTrue(TEXT("History opens only as the online local presentation route"),
        View.Screen == EEchoesShellScreen::FeedbackHistory &&
        View.Status.ToString().Contains(TEXT("ONLINE MATCH CONTINUES")));
    TestFalse(TEXT("History retains the running online simulation"),
        Bridge->IsScenarioPaused());
    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("History Back returns to the online local menu"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Gameplay &&
        Controller->IsOnlineLocalMenuVisible());

    // The presentation helper refuses arbitrary screens and nested routes, so
    // a field menu cannot tunnel into replay or bypass its own action model.
    TestFalse(TEXT("Online menu refuses non-local shell routes"),
        Controller->OpenOnlineLocalMenuShellScreen(
            EEchoesShellScreen::ReplayBrowser));

    Bridge->SetNetworkHumanOpponent(false);
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
