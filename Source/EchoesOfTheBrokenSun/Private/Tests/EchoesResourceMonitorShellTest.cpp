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
bool HasShellAction(const FEchoesShellView& View, EEchoesShellAction Action)
{
    return View.Buttons.ContainsByPredicate(
        [Action](const FEchoesShellButton& Button)
        {
            return Button.Action == Action && Button.bEnabled;
        });
}

bool HasOnlineResourceControl(
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
    FEchoesResourceMonitorShellTest,
    "Echoes.Runtime.UI.ResourceMonitorShellRoutes",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesResourceMonitorShellTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the resource-monitor shell test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestTrue(TEXT("Scenario starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Resource-monitor route owner exists"), Controller))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // The live monitor lives behind Pause, so its resource snapshot cannot be
    // read as an active-field overlay.
    Controller->TogglePauseMenu();
    TestTrue(TEXT("Pause owns the local monitor route"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Pause &&
            Bridge->IsScenarioPaused());
    TestTrue(TEXT("Pause exposes the resource monitor"),
        HasShellAction(
            Controller->BuildShellView(),
            EEchoesShellAction::OpenResourceMonitor));

    Controller->HandleShellAction(EEchoesShellAction::OpenResourceMonitor);
    FEchoesShellView View = Controller->BuildShellView();
    TestTrue(TEXT("Paused live match opens the resource monitor"),
        View.Screen == EEchoesShellScreen::ResourceMonitor &&
            View.Title.ToString().Contains(TEXT("Resource monitor")) &&
            HasShellAction(View, EEchoesShellAction::Back));
    TestTrue(TEXT("Live monitor reports its scoped state instead of an invented estimate"),
        View.Status.ToString().Contains(TEXT("Match paused")) &&
            View.Body.ToString().Contains(TEXT("AVAILABLE NOW")));

    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("Monitor Back restores the paused local menu"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Pause &&
            Bridge->IsScenarioPaused());
    Controller->TogglePauseMenu();
    TestEqual(TEXT("Resuming returns the local route to gameplay"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::Gameplay);

    // The online local menu uses the same presentation route without taking
    // scenario authority. This fixture keeps the bridge running intentionally.
    Bridge->SetNetworkHumanOpponent(true);
    Controller->bNetworkCompatibilityAccepted = true;
    Controller->bNetworkMatchStarted = true;
    Controller->NetworkSeat = UEchoesSimulationSubsystem::LocalPlayerId;
    Controller->bOnlineLocalMenuVisible = true;
    const FEchoesFieldHudView OnlineView = Controller->BuildFieldHudView();
    TestTrue(TEXT("Online local menu exposes the resource monitor"),
        OnlineView.Surface == EEchoesFieldHudSurface::OnlineLocalMenu &&
            HasOnlineResourceControl(OnlineView, EEchoesFieldHudAction::OpenResourceMonitor));
    TestFalse(TEXT("Online local menu keeps the simulation running"),
        Bridge->IsScenarioPaused());

    Controller->HandleFieldHudAction(EEchoesFieldHudAction::OpenResourceMonitor);
    View = Controller->BuildShellView();
    TestTrue(TEXT("Online local menu opens its monitor route"),
        View.Screen == EEchoesShellScreen::ResourceMonitor &&
            View.Status.ToString().Contains(TEXT("ONLINE MATCH CONTINUES")));
    TestFalse(TEXT("Online monitor never pauses the scenario"),
        Bridge->IsScenarioPaused());
    Controller->HandleShellAction(EEchoesShellAction::Back);
    TestTrue(TEXT("Online monitor Back restores the live online local menu"),
        Controller->GetPlayerFlow().Current() == EEchoesShellScreen::Gameplay &&
            Controller->IsOnlineLocalMenuVisible() &&
            !Bridge->IsScenarioPaused());

    Bridge->SetNetworkHumanOpponent(false);
    Controller->bNetworkCompatibilityAccepted = false;
    Controller->bNetworkMatchStarted = false;
    Controller->bOnlineLocalMenuVisible = false;

    Controller->PresentTitleScreen();
    TestFalse(TEXT("Title refuses the match-only monitor"),
        Controller->OpenResourceMonitor());
    TestEqual(TEXT("Title remains the current route after refusal"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::Title);
    Controller->HandleShellAction(EEchoesShellAction::OpenReplayBrowser);
    TestEqual(TEXT("Replay browser opens from title"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::ReplayBrowser);
    TestFalse(TEXT("Replay browser refuses the live resource monitor"),
        Controller->OpenResourceMonitor());
    TestEqual(TEXT("Replay browser remains current after monitor refusal"),
        Controller->GetPlayerFlow().Current(), EEchoesShellScreen::ReplayBrowser);

    Bridge->StopPrototypeScenario();
    FEchoesShellView Unavailable;
    Controller->BuildResourceMonitorShellView(Unavailable);
    TestTrue(TEXT("Unavailable source produces a safe monitor body"),
        Unavailable.Body.ToString().Contains(TEXT("unavailable")));

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
