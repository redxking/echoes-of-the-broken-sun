#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
bool IsAnchorSelectionInstruction(const FText& Instruction)
{
    return Instruction.ToString().Contains(TEXT("on your Anchor to begin"));
}

uint32 FindLocalCommandCore(const UEchoesSimulationSubsystem& Bridge)
{
    const auto* Simulation = Bridge.GetSimulation();
    if (Simulation == nullptr) return 0;
    for (const auto& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore)
        {
            return Entity.id;
        }
    }
    return 0;
}
}

/**
 * Reproduces the rendered player-route defect: the readiness survey stayed on
 * "select your Anchor" after [ECHOES_POINTER_SELECTION] proved the owned Core
 * was selected. Exercises the same controller observation the pointer path
 * calls, on the same fixed-step cadence the runtime uses.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialAnchorSelectionTest,
    "Echoes.Runtime.Campaign.TutorialAnchorSelectionProgression",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialAnchorSelectionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the anchor-selection test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("World owns the simulation subsystem"), Bridge)) return false;

    FString Feedback;
    if (!TestTrue(TEXT("Training readiness can be selected"),
            Bridge->SelectOperationMode(EEchoesOperationMode::TrainingReadiness, Feedback)) ||
        !TestTrue(TEXT("Training readiness starts"), Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    auto* Controller = World->SpawnActor<AEchoesPlayerController>();
    auto* Camera = World->SpawnActor<AEchoesRTSCameraPawn>();
    if (!TestNotNull(TEXT("Controller spawns"), Controller) ||
        !TestNotNull(TEXT("RTS camera spawns"), Camera))
    {
        if (Controller) Controller->Destroy();
        if (Camera) Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->InitInputSystem();
    Controller->Possess(Camera);

    // Same state the deployment route leaves behind before the first fixed step.
    Controller->bTutorialOperationAuthorized = true;
    Controller->bPlayerProfileAvailable = true;
    Controller->PlayerProfile.TutorialVerifiedMask = 0;
    Controller->ResetTutorialObservation();
    TestTrue(TEXT("Deployment frames the owned base"), Camera->CenterOnLocalBase());
    Bridge->SetScenarioPaused(false);

    const uint32 CoreId = FindLocalCommandCore(*Bridge);
    TestTrue(TEXT("Training scenario owns a local Command Core"), CoreId != 0);

    auto Step = [&]()
    {
        Bridge->Tick(0.05f);
        Controller->TickTutorialObservation();
    };

    Step();
    TestTrue(TEXT("Survey opens on the Anchor-selection instruction"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));
    TestEqual(TEXT("Survey binds the owned Core the deployment centered"),
        Controller->TutorialCoreId, CoreId);
    TestTrue(TEXT("Survey observation is active before the click"),
        Controller->GetTutorialSurvey().IsActive());
    TestFalse(TEXT("No selection is observed before the click"),
        Controller->IsTutorialCoreSelected());

    // The pointer path (SelectAtCursor) records the selection exactly this way
    // after the trace resolves the owned entity view.
    Controller->SelectedEntityIds.Reset();
    Controller->SelectedEntityIds.Add(CoreId);
    Controller->ObserveTutorialSelection(CoreId, false);
    Controller->ObserveTutorialSelectionEvent(
        EEchoesTutorialSelectionInputEvent::SingleClickSelection);
    TestTrue(TEXT("Anchor click is observed immediately"),
        Controller->IsTutorialCoreSelected());

    Step();
    TestTrue(TEXT("Anchor selection survives the next fixed step"),
        Controller->IsTutorialCoreSelected());
    TestFalse(TEXT("Instruction advances past Anchor selection after the click"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));

    // Idle ticks with the camera stationary must not regress the lesson.
    for (int32 Index = 0; Index < 40; ++Index) Step();
    TestTrue(TEXT("Anchor selection survives idle fixed steps"),
        Controller->IsTutorialCoreSelected());
    TestFalse(TEXT("Instruction stays advanced while idle"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));

    // Player-driven scrolling (keyboard/edge) is the taught next action.
    Camera->SetForwardInput(1.0f);
    for (int32 Index = 0; Index < 10; ++Index)
    {
        Camera->Tick(0.05f);
        Step();
    }
    Camera->SetForwardInput(0.0f);
    TestTrue(TEXT("Player scrolling counts as navigation"), Camera->WasLastNavigationPlayerDriven());
    TestTrue(TEXT("Anchor selection survives player scrolling"),
        Controller->IsTutorialCoreSelected());
    TestTrue(TEXT("Survey observation survives player scrolling"),
        Controller->GetTutorialSurvey().IsActive());
    TestFalse(TEXT("Instruction stays advanced after player scrolling"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));

    // A cursor-centred wheel zoom is player navigation as well.
    Camera->ApplyZoomAtViewportPoint(1.0f, FVector2D(640, 360), FVector2D(1280, 720));
    Step();
    TestTrue(TEXT("Anchor selection survives player zoom"),
        Controller->IsTutorialCoreSelected());
    TestFalse(TEXT("Instruction stays advanced after player zoom"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));

    // The taught recenter key (SnapKeyboardTargetToSelection) must be honest
    // player navigation the survey can observe, not a silent teleport.
    TestNotNull(TEXT("Owned Core has a presentation view for the recenter key"),
        Bridge->FindEntityView(CoreId));
    const uint64 RevisionBeforeRecenter = Camera->GetNavigationRevision();
    Controller->SnapKeyboardTargetToSelection();
    TestTrue(TEXT("Recenter key records a navigation revision"),
        Camera->GetNavigationRevision() != RevisionBeforeRecenter);
    TestTrue(TEXT("Recenter key is player-driven navigation"),
        Camera->WasLastNavigationPlayerDriven());
    Step();
    TestTrue(TEXT("Anchor selection survives the recenter key"),
        Controller->IsTutorialCoreSelected());
    TestTrue(TEXT("Survey observation survives the recenter key"),
        Controller->GetTutorialSurvey().IsActive());
    TestFalse(TEXT("Instruction stays advanced after the recenter key"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));

    // A programmatic recentre (deployment framing, restore) is not player
    // navigation. It may restart the camera observation but must not erase the
    // verified Anchor selection or re-demand a click the player already made.
    Camera->CenterOnLocalBase();
    Step();
    Step();
    TestTrue(TEXT("Anchor selection survives a programmatic camera move"),
        Controller->IsTutorialCoreSelected());
    TestFalse(TEXT("Instruction stays advanced after a programmatic camera move"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));
    TestTrue(TEXT("Camera observation restarts after a programmatic move"),
        Controller->GetTutorialSurvey().IsActive());

    // Authority changes still clear the selection gate: a restarted scenario
    // must re-demand the click rather than carry stale evidence forward.
    Controller->ResetTutorialObservation();
    TestFalse(TEXT("Observer reset clears the Anchor selection"),
        Controller->IsTutorialCoreSelected());
    Step();
    TestTrue(TEXT("Survey re-demands Anchor selection after a full reset"),
        IsAnchorSelectionInstruction(Controller->GetTutorialInstruction()));

    Controller->Destroy();
    Camera->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
