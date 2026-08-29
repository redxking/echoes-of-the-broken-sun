#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/InputComponent.h"
#include "EchoesGameMode.h"
#include "EchoesFogView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesHUD.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "EchoesWeatherView.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

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
    TestNotNull(TEXT("Echoes user settings are registered"), UEchoesGameUserSettings::StaticClass());
    TestNotNull(TEXT("Echoes fog view is registered"), AEchoesFogView::StaticClass());
    TestNotNull(TEXT("Echoes terrain view is registered"), AEchoesTerrainView::StaticClass());
    TestNotNull(TEXT("Echoes weather view is registered"), AEchoesWeatherView::StaticClass());
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
        TestFalse(
            TEXT("RTS pointer is not captured on launch"),
            InputSettings->bCaptureMouseOnLaunch);
        TestEqual(
            TEXT("RTS pointer remains uncaptured during clicks"),
            InputSettings->DefaultViewportMouseCaptureMode,
            EMouseCaptureMode::NoCapture);
        TestEqual(
            TEXT("RTS pointer is not locked to the viewport"),
            InputSettings->DefaultViewportMouseLockMode,
            EMouseLockMode::DoNotLock);
        const TArray<FInputActionKeyMapping>& ActionMappings =
            InputSettings->GetActionMappings();
        const auto HasAction = [&ActionMappings](
                                   FName Action,
                                   const FKey& Key,
                                   bool bControl = false,
                                   bool bShift = false)
        {
            return ActionMappings.ContainsByPredicate(
                [Action, Key, bControl, bShift](const FInputActionKeyMapping& Mapping)
                {
                    return Mapping.ActionName == Action && Mapping.Key == Key &&
                           Mapping.bCtrl == bControl && Mapping.bShift == bShift;
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
        TestTrue(TEXT("Patrol input is mapped"),
                 HasAction(TEXT("PatrolAtCursor"), EKeys::T));
        TestTrue(TEXT("Hold-position input is mapped"),
                 HasAction(TEXT("HoldSelected"), EKeys::H));
        TestTrue(TEXT("Guard input is mapped"),
                 HasAction(TEXT("GuardAtCursor"), EKeys::J));
        TestTrue(TEXT("Stop input is mapped"),
                 HasAction(TEXT("StopSelected"), EKeys::X));
        TestTrue(TEXT("Pause input is mapped"),
                 HasAction(TEXT("PauseScenario"), EKeys::P));
        TestTrue(TEXT("Escape field-menu input is mapped"),
                 HasAction(TEXT("PauseScenario"), EKeys::Escape));
        TestTrue(TEXT("Restart input is mapped"),
                 HasAction(TEXT("RestartScenario"), EKeys::R));
        TestTrue(TEXT("Quick-save input is mapped"),
                 HasAction(TEXT("QuickSaveScenario"), EKeys::K));
        TestTrue(TEXT("Quick-load input is mapped"),
                 HasAction(TEXT("QuickLoadScenario"), EKeys::L));
        TestTrue(TEXT("Future Well harvest input is remapped"),
                 HasAction(TEXT("ChooseHarvest"), EKeys::Z));
        TestTrue(TEXT("Future Well preserve input is remapped"),
                 HasAction(TEXT("ChoosePreserve"), EKeys::C));
        TestTrue(TEXT("Future Well reshape input is remapped"),
                 HasAction(TEXT("ChooseReshape"), EKeys::V));
        TestTrue(TEXT("Control-group recall input is mapped"),
                 HasAction(TEXT("RecallControlGroup1"), EKeys::One));
        TestTrue(TEXT("Control-group assignment input is mapped"),
                 HasAction(TEXT("ArmControlGroupAssignment"), EKeys::G));
        TestTrue(TEXT("Control-group zero recall input is mapped"),
                 HasAction(TEXT("RecallControlGroup0"), EKeys::Zero));
        TestTrue(TEXT("HUD-scale accessibility input is mapped"),
                 HasAction(TEXT("CycleHudScale"), EKeys::U));
        TestTrue(TEXT("High-contrast accessibility input is mapped"),
                 HasAction(TEXT("ToggleHighContrast"), EKeys::I));
        TestTrue(TEXT("Reduced-motion accessibility input is mapped"),
                 HasAction(TEXT("ToggleReducedMotion"), EKeys::O));
        TestTrue(TEXT("Reduced-flashing accessibility input is mapped"),
                 HasAction(TEXT("ToggleReducedFlashing"), EKeys::Slash));
        TestTrue(TEXT("Edge-pan preference input is mapped"),
                 HasAction(TEXT("ToggleEdgePan"), EKeys::Y));
        TestTrue(TEXT("Camera pan decrease input is mapped"),
                 HasAction(TEXT("DecreaseCameraPanSpeed"), EKeys::LeftBracket));
        TestTrue(TEXT("Camera pan increase input is mapped"),
                 HasAction(TEXT("IncreaseCameraPanSpeed"), EKeys::RightBracket));
        TestTrue(TEXT("Camera zoom decrease input is mapped"),
                 HasAction(TEXT("DecreaseCameraZoomSpeed"), EKeys::Comma));
        TestTrue(TEXT("Camera zoom increase input is mapped"),
                 HasAction(TEXT("IncreaseCameraZoomSpeed"), EKeys::Period));
        TestTrue(TEXT("Primary modal confirmation input is mapped"),
                 HasAction(TEXT("ConfirmPrimaryAction"), EKeys::Enter));
        TestTrue(TEXT("Playable-faction cycle input is mapped"),
                 HasAction(TEXT("CyclePlayableFaction"), EKeys::Tab));
        TestTrue(TEXT("Faction research input is mapped"),
                 HasAction(TEXT("ResearchNext"), EKeys::R, false, true));
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

    FTestWorldWrapper WeatherWorld;
    if (WeatherWorld.CreateTestWorld(EWorldType::Game))
    {
        AEchoesWeatherView* Weather =
            WeatherWorld.GetTestWorld()->SpawnActor<AEchoesWeatherView>();
        if (TestNotNull(TEXT("Glass Scar atmosphere can be instantiated"), Weather))
        {
            UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
            const bool bPreviousReducedMotion =
                Settings != nullptr && Settings->IsReducedMotionEnabled();
            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(true);
                Weather->Tick(7.0f);
                const float ReducedMotionDensity = Weather->GetCurrentFogDensity();
                Weather->Tick(7.0f);
                TestEqual(
                    TEXT("Reduced motion holds the atmospheric density steady"),
                    Weather->GetCurrentFogDensity(),
                    ReducedMotionDensity);
                Settings->SetReducedMotionEnabled(false);
                Weather->Tick(7.0f);
                TestNotEqual(
                    TEXT("Standard presentation advances the atmospheric drift"),
                    Weather->GetCurrentFogDensity(),
                    ReducedMotionDensity);
                Settings->SetReducedMotionEnabled(bPreviousReducedMotion);
            }
        }
        WeatherWorld.ForwardErrorMessages(this);
    }
    else
    {
        WeatherWorld.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the atmospheric test world."));
    }

    return !HasAnyErrors();
}

#endif
