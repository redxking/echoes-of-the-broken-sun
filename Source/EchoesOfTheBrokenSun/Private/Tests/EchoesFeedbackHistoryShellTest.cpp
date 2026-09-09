// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
bool HasAction(const FEchoesShellView& View, EEchoesShellAction Action)
{
    return View.Buttons.ContainsByPredicate(
        [Action](const FEchoesShellButton& Button)
        {
            return Button.Action == Action && Button.bEnabled;
        });
}

bool IsSelectedFilter(
    const FEchoesShellView& View,
    EEchoesShellAction Action,
    const FString& ExpectedLabel)
{
    const FEchoesShellButton* Button = View.Buttons.FindByPredicate(
        [Action](const FEchoesShellButton& Candidate)
        {
            return Candidate.Action == Action;
        });
    return Button != nullptr &&
        Button->Label.ToString().Contains(TEXT("Selected")) &&
        Button->Label.ToString().Contains(ExpectedLabel);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFeedbackHistoryShellTest,
    "Echoes.Runtime.UI.FeedbackHistoryShellRoutes",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFeedbackHistoryShellTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the feedback-history shell test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    // The profile must initialize before the scenario runs. SelectJourneySlot
    // refuses while a scenario is ready and unpaused, and this fixture keeps it
    // running on purpose, so a lazy initialization during the first shell action
    // would push an Error screen into the middle of the route under test.
    AEchoesPlayerController* Controller = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Feedback history route owner exists"), Controller))
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


    // The live-match history route must not be reachable while the field is
    // active. It is deliberately a paused-menu view, never a HUD overlay.
    Controller->HandleShellAction(EEchoesShellAction::OpenFeedbackHistory);
    TestEqual(TEXT("Running live match refuses command history"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::Gameplay);

    Controller->TogglePauseMenu();
    TestTrue(TEXT("Pause owns the live-match menu and authoritative pause"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Pause &&
            Bridge->IsScenarioPaused());
    TestTrue(TEXT("Pause exposes command history"),
        HasAction(Controller->BuildShellView(), EEchoesShellAction::OpenFeedbackHistory));

    Controller->HandleShellAction(EEchoesShellAction::OpenFeedbackHistory);
    FEchoesShellView View = Controller->BuildShellView();
    TestEqual(TEXT("Pause opens the history route"),
        View.Screen, EEchoesShellScreen::FeedbackHistory);
    TestTrue(TEXT("History exposes every filter"),
        HasAction(View, EEchoesShellAction::FeedbackHistoryAll) &&
        HasAction(View, EEchoesShellAction::FeedbackHistoryOrders) &&
        HasAction(View, EEchoesShellAction::FeedbackHistoryConstruction) &&
        HasAction(View, EEchoesShellAction::FeedbackHistoryProduction));
    TestTrue(TEXT("History begins at the all-events filter"),
        IsSelectedFilter(View, EEchoesShellAction::FeedbackHistoryAll, TEXT("All")));

    const struct
    {
        EEchoesShellAction Action;
        const TCHAR* Label;
    } Filters[] = {
        {EEchoesShellAction::FeedbackHistoryOrders, TEXT("Orders")},
        {EEchoesShellAction::FeedbackHistoryConstruction, TEXT("Construction")},
        {EEchoesShellAction::FeedbackHistoryProduction, TEXT("Production")},
        {EEchoesShellAction::FeedbackHistoryAll, TEXT("All")}
    };
    for (const auto& Filter : Filters)
    {
        Controller->HandleShellAction(Filter.Action);
        View = Controller->BuildShellView();
        TestTrue(*FString::Printf(TEXT("%s filter remains on command history"), Filter.Label),
            View.Screen == EEchoesShellScreen::FeedbackHistory);
        TestTrue(*FString::Printf(TEXT("%s filter becomes selected"), Filter.Label),
            IsSelectedFilter(View, Filter.Action, Filter.Label));
    }

    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestEqual(TEXT("History Back returns to the same paused live menu"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::Pause);
    TestTrue(TEXT("History Back preserves authoritative pause"), Bridge->IsScenarioPaused());

    // Replay browser is a distinct authority and must never expose the live
    // feedback stream, even though it is a shell route on the same controller.
    Controller->PresentTitleScreen();
    Controller->HandleShellAction(EEchoesShellAction::OpenReplayBrowser);
    TestEqual(TEXT("Replay browser opens from the title"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::ReplayBrowser);
    Controller->HandleShellAction(EEchoesShellAction::OpenFeedbackHistory);
    TestEqual(TEXT("Replay browser refuses live command history"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::ReplayBrowser);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
