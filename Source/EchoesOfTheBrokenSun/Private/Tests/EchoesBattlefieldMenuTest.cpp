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
bool HasMenuAction(const FEchoesFieldHudView& View)
{
    return View.Menu.bVisible &&
        View.Menu.Control.bEnabled &&
        View.Menu.Control.Action == EEchoesFieldHudAction::OpenPauseMenu;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesBattlefieldMenuTest,
    "Echoes.Runtime.UI.BattlefieldMenuAffordance",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesBattlefieldMenuTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the battlefield-menu test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestTrue(TEXT("Scenario starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Battlefield menu route owner exists"), Controller))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Live battlefield publishes a semantic Menu action"),
        HasMenuAction(Controller->BuildFieldHudView()));
    TestFalse(TEXT("Menu starts without an authoritative pause"),
        Bridge->IsScenarioPaused());

    Controller->HandleFieldHudAction(EEchoesFieldHudAction::OpenPauseMenu);
    TestTrue(TEXT("Menu reuses the existing pause route"),
        Controller->IsPauseMenuVisible() && Bridge->IsScenarioPaused());
    TestFalse(TEXT("Menu is absent while the pause route owns the modal"),
        HasMenuAction(Controller->BuildFieldHudView()));

    // Resume through the existing controller route and verify the same control
    // becomes available again; no parallel pause mechanism exists here.
    Controller->TogglePauseMenu();
    TestTrue(TEXT("Resume restores the battlefield Menu action"),
        !Bridge->IsScenarioPaused() && HasMenuAction(Controller->BuildFieldHudView()));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
