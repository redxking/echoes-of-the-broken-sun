#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/InputComponent.h"
#include "EchoesGameMode.h"
#include "EchoesFogView.h"
#include "EchoesHUD.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesRuntimeSmokeTest,
    "Echoes.Runtime.Bootstrap.ClassesAndCore",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesRuntimeSmokeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    TestNotNull(TEXT("Echoes GameMode is registered"), AEchoesGameMode::StaticClass());
    TestNotNull(TEXT("Echoes player controller is registered"), AEchoesPlayerController::StaticClass());
    TestNotNull(TEXT("Echoes camera pawn is registered"), AEchoesRTSCameraPawn::StaticClass());
    TestNotNull(TEXT("Echoes HUD is registered"), AEchoesHUD::StaticClass());
    TestNotNull(TEXT("Echoes fog view is registered"), AEchoesFogView::StaticClass());
    TestNotNull(TEXT("Echoes terrain view is registered"), AEchoesTerrainView::StaticClass());
    TestNotNull(TEXT("Echoes simulation subsystem is registered"), UEchoesSimulationSubsystem::StaticClass());

    const UInputSettings* InputSettings = GetDefault<UInputSettings>();
    TestNotNull(TEXT("Input settings are available"), InputSettings);
    if (InputSettings != nullptr)
    {
        TestTrue(
            TEXT("Legacy PlayerInput is explicitly selected"),
            InputSettings->GetDefaultPlayerInputClass() == UPlayerInput::StaticClass());
        TestTrue(
            TEXT("Legacy InputComponent is explicitly selected"),
            InputSettings->GetDefaultInputComponentClass() == UInputComponent::StaticClass());
        const TArray<FInputActionKeyMapping>& ActionMappings =
            InputSettings->GetActionMappings();
        const auto HasAction = [&ActionMappings](
                                   FName Action,
                                   const FKey& Key,
                                   bool bControl = false)
        {
            return ActionMappings.ContainsByPredicate(
                [Action, Key, bControl](const FInputActionKeyMapping& Mapping)
                {
                    return Mapping.ActionName == Action && Mapping.Key == Key &&
                           Mapping.bCtrl == bControl;
                });
        };
        TestTrue(TEXT("Barracks construction input is mapped"),
                 HasAction(TEXT("BuildBarracks"), EKeys::B));
        TestTrue(TEXT("Drop-off construction input is mapped"),
                 HasAction(TEXT("BuildDropoff"), EKeys::N));
        TestTrue(TEXT("Worker production input is mapped"),
                 HasAction(TEXT("ProduceWorker"), EKeys::Q));
        TestTrue(TEXT("Soldier production input is mapped"),
                 HasAction(TEXT("ProduceSoldier"), EKeys::E));
        TestTrue(TEXT("Attack-move input is mapped"),
                 HasAction(TEXT("AttackMoveAtCursor"), EKeys::F));
        TestTrue(TEXT("Stop input is mapped"),
                 HasAction(TEXT("StopSelected"), EKeys::X));
        TestTrue(TEXT("Pause input is mapped"),
                 HasAction(TEXT("PauseScenario"), EKeys::P));
        TestTrue(TEXT("Restart input is mapped"),
                 HasAction(TEXT("RestartScenario"), EKeys::R));
        TestTrue(TEXT("Quick-save input is mapped"),
                 HasAction(TEXT("QuickSaveScenario"), EKeys::K));
        TestTrue(TEXT("Quick-load input is mapped"),
                 HasAction(TEXT("QuickLoadScenario"), EKeys::L));
    }

    echoes::sim::SimulationConfig Config;
    Config.mapWidthTiles = 16;
    Config.mapHeightTiles = 16;
    Config.ticksPerSecond = 20;
    Config.randomSeed = 0xE0C0B5A1ULL;

    echoes::sim::Simulation Simulation(Config);
    const bool bPlayerAdded = Simulation.AddPlayer(
        0,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::ResourcePool{100, 5});
    const echoes::sim::EntityId Worker = Simulation.SpawnEntity(
        0,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::Worker,
        echoes::sim::Vec2::FromTiles(2, 2));
    Simulation.Step();

    TestTrue(TEXT("Portable simulation accepts an Unreal-hosted player"), bPlayerAdded);
    TestTrue(TEXT("Portable simulation exports callable entity creation"), Worker != 0);
    TestTrue(TEXT("Portable simulation advances one deterministic tick"), Simulation.CurrentTick() == 1);
    TestTrue(TEXT("Portable simulation exposes a nonzero state checksum"), Simulation.StateChecksum() != 0);

    return true;
}

#endif
