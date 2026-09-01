#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "Components/InputComponent.h"
#include "EchoesPlayerController.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPlayerDispatchState final
{
    bool bTitleVisible = false;
    bool bBriefingVisible = false;
    bool bOnlineFrontDoorVisible = false;
    bool bModalVisible = false;
    bool bScenarioPaused = false;
    EEchoesFormationType Formation = EEchoesFormationType::Box;
    echoes::sim::FutureWellChoice FutureWellChoice =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesOperationMode Operation = EEchoesOperationMode::Skirmish;
    uint64 SimulationTick = 0;
    uint64 SimulationChecksum = 0;
    int32 EntityCount = 0;

    [[nodiscard]] bool operator==(const FPlayerDispatchState& Other) const
    {
        return bTitleVisible == Other.bTitleVisible &&
            bBriefingVisible == Other.bBriefingVisible &&
            bOnlineFrontDoorVisible == Other.bOnlineFrontDoorVisible &&
            bModalVisible == Other.bModalVisible &&
            bScenarioPaused == Other.bScenarioPaused &&
            Formation == Other.Formation &&
            FutureWellChoice == Other.FutureWellChoice &&
            Operation == Other.Operation &&
            SimulationTick == Other.SimulationTick &&
            SimulationChecksum == Other.SimulationChecksum &&
            EntityCount == Other.EntityCount;
    }

    [[nodiscard]] FString Describe() const
    {
        return FString::Printf(
            TEXT("title=%s briefing=%s online=%s modal=%s paused=%s ")
            TEXT("formation=%d well=%d operation=%d tick=%llu ")
            TEXT("checksum=%llu entities=%d"),
            bTitleVisible ? TEXT("true") : TEXT("false"),
            bBriefingVisible ? TEXT("true") : TEXT("false"),
            bOnlineFrontDoorVisible ? TEXT("true") : TEXT("false"),
            bModalVisible ? TEXT("true") : TEXT("false"),
            bScenarioPaused ? TEXT("true") : TEXT("false"),
            static_cast<int32>(Formation),
            static_cast<int32>(FutureWellChoice),
            static_cast<int32>(Operation),
            static_cast<unsigned long long>(SimulationTick),
            static_cast<unsigned long long>(SimulationChecksum),
            EntityCount);
    }
};

struct FDispatchRun final
{
    FPlayerDispatchState Before;
    FPlayerDispatchState After;
};

[[nodiscard]] FPlayerDispatchState CaptureDispatchState(
    const AEchoesPlayerController& Controller,
    const UEchoesSimulationSubsystem& Bridge)
{
    FPlayerDispatchState State;
    State.bTitleVisible = Controller.IsTitleScreenVisible();
    State.bBriefingVisible = Controller.IsMissionBriefingVisible();
    State.bOnlineFrontDoorVisible = Controller.IsOnlineFrontDoorVisible();
    State.bModalVisible = Controller.IsModalOverlayVisible();
    State.bScenarioPaused = Bridge.IsScenarioPaused();
    State.Formation = Controller.GetFormationType();
    State.FutureWellChoice = Controller.GetFutureWellChoice();
    State.Operation = Bridge.GetOperationMode();
    if (const echoes::sim::Simulation* Simulation = Bridge.GetSimulation())
    {
        State.SimulationTick = Simulation->CurrentTick();
        State.SimulationChecksum = Simulation->StateChecksum();
        State.EntityCount = static_cast<int32>(Simulation->Entities().size());
    }
    return State;
}

[[nodiscard]] const FInputActionBinding* FindUniquePressedControllerBinding(
    FAutomationTestBase& Test,
    UInputComponent& InputComponent,
    AEchoesPlayerController& Controller,
    const FName ActionName,
    const TCHAR* Scope)
{
    int32 PressedBindingCount = 0;
    int32 ControllerBoundCount = 0;
    const FInputActionBinding* ControllerBinding = nullptr;
    for (int32 Index = 0; Index < InputComponent.GetNumActionBindings(); ++Index)
    {
        const FInputActionBinding& Binding =
            InputComponent.GetActionBinding(Index);
        if (Binding.GetActionName() != ActionName ||
            Binding.KeyEvent != IE_Pressed)
        {
            continue;
        }

        ++PressedBindingCount;
        if (Binding.ActionDelegate.IsBound() &&
            Binding.ActionDelegate.IsBoundToObject(&Controller))
        {
            ++ControllerBoundCount;
            ControllerBinding = &Binding;
        }
    }

    const FString PressedLabel = FString::Printf(
        TEXT("%s has exactly one pressed binding for %s"),
        Scope,
        *ActionName.ToString());
    const FString ControllerLabel = FString::Printf(
        TEXT("%s has exactly one controller-bound delegate for %s"),
        Scope,
        *ActionName.ToString());
    Test.TestEqual(*PressedLabel, PressedBindingCount, 1);
    Test.TestEqual(*ControllerLabel, ControllerBoundCount, 1);
    return PressedBindingCount == 1 && ControllerBoundCount == 1
        ? ControllerBinding
        : nullptr;
}

[[nodiscard]] TArray<FName> VerifySharedKeyMappings(
    FAutomationTestBase& Test,
    const UInputSettings& InputSettings,
    const FKey& Key,
    const FName FirstExpectedAction,
    const FName SecondExpectedAction)
{
    TArray<FName> Actions;
    int32 PhysicalKeyMappingCount = 0;
    bool bEveryMappingUnmodified = true;
    for (const FInputActionKeyMapping& Mapping :
         InputSettings.GetActionMappings())
    {
        if (Mapping.Key != Key)
        {
            continue;
        }

        ++PhysicalKeyMappingCount;
        const bool bUnmodified = !Mapping.bAlt && !Mapping.bCmd &&
            !Mapping.bCtrl && !Mapping.bShift;
        bEveryMappingUnmodified &= bUnmodified;
        if (bUnmodified)
        {
            Actions.Add(Mapping.ActionName);
        }
    }

    const FString CountLabel = FString::Printf(
        TEXT("%s has exactly two action mappings"),
        *Key.GetDisplayName().ToString());
    const FString ModifierLabel = FString::Printf(
        TEXT("%s shared mappings are unmodified"),
        *Key.GetDisplayName().ToString());
    const FString FirstLabel = FString::Printf(
        TEXT("%s maps %s"),
        *Key.GetDisplayName().ToString(),
        *FirstExpectedAction.ToString());
    const FString SecondLabel = FString::Printf(
        TEXT("%s maps %s"),
        *Key.GetDisplayName().ToString(),
        *SecondExpectedAction.ToString());
    Test.TestEqual(*CountLabel, PhysicalKeyMappingCount, 2);
    Test.TestTrue(*ModifierLabel, bEveryMappingUnmodified);
    Test.TestTrue(*FirstLabel, Actions.Contains(FirstExpectedAction));
    Test.TestTrue(*SecondLabel, Actions.Contains(SecondExpectedAction));
    return Actions;
}

[[nodiscard]] bool RunComponentDelegateSequence(
    FAutomationTestBase& Test,
    const TCHAR* Scope,
    const FKey& PhysicalKey,
    const TArray<FName>& MappingOrder,
    const bool bReverseOrder,
    const bool bPresentTitle,
    FDispatchRun& OutRun)
{
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(&Test);
        Test.AddError(FString::Printf(
            TEXT("%s could not create its test world."),
            Scope));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (Bridge == nullptr || !Bridge->StartPrototypeScenario())
    {
        Test.AddError(FString::Printf(
            TEXT("%s could not start the prototype scenario."),
            Scope));
        WorldWrapper.ForwardErrorMessages(&Test);
        return false;
    }

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (Controller == nullptr)
    {
        Test.AddError(FString::Printf(
            TEXT("%s could not spawn the player controller."),
            Scope));
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(&Test);
        return false;
    }

    Controller->SetupInputComponent();
    UInputComponent* InputComponent = Controller->InputComponent;
    if (InputComponent == nullptr)
    {
        Test.AddError(FString::Printf(
            TEXT("%s did not create an InputComponent."),
            Scope));
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(&Test);
        return false;
    }

    if (bPresentTitle)
    {
        Controller->PresentTitleScreen();
        if (!Controller->IsTitleScreenVisible())
        {
            Test.AddError(FString::Printf(
                TEXT("%s did not enter the title context."),
                Scope));
        }
    }
    else if (Controller->IsModalOverlayVisible())
    {
        Test.AddError(FString::Printf(
            TEXT("%s did not begin in the battlefield context."),
            Scope));
    }

    OutRun.Before = CaptureDispatchState(*Controller, *Bridge);
    bool bExecutedEveryDelegate = true;
    for (int32 Step = 0; Step < MappingOrder.Num(); ++Step)
    {
        const int32 MappingIndex = bReverseOrder
            ? MappingOrder.Num() - Step - 1
            : Step;
        const FName ActionName = MappingOrder[MappingIndex];
        const FInputActionBinding* Binding =
            FindUniquePressedControllerBinding(
                Test,
                *InputComponent,
                *Controller,
                ActionName,
                Scope);
        if (Binding == nullptr)
        {
            bExecutedEveryDelegate = false;
            break;
        }
        Binding->ActionDelegate.Execute(PhysicalKey);
    }
    OutRun.After = CaptureDispatchState(*Controller, *Bridge);

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(&Test);
    return bExecutedEveryDelegate && !WorldWrapper.HasFailed();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPlayerInputCollisionTest,
    "Echoes.Runtime.Controls.SharedKeyDispatch",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPlayerInputCollisionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    AddInfo(
        TEXT("Scope: this test executes real controller-bound InputComponent ")
        TEXT("delegates directly. It does not call UPlayerInput::ProcessInputStack ")
        TEXT("and does not claim end-to-end physical-key dispatch. The standard ")
        TEXT("test world does not install the project GameInstance, so ")
        TEXT("online-front-door transition is not asserted."));

    const UInputSettings* InputSettings = GetDefault<UInputSettings>();
    if (!TestNotNull(TEXT("Input settings are available"), InputSettings))
    {
        return false;
    }

    const TArray<FName> CMappingOrder = VerifySharedKeyMappings(
        *this,
        *InputSettings,
        EKeys::C,
        TEXT("ChoosePreserve"),
        TEXT("ContinueCampaign"));
    const TArray<FName> F8MappingOrder = VerifySharedKeyMappings(
        *this,
        *InputSettings,
        EKeys::F8,
        TEXT("OpenOnlineFrontDoor"),
        TEXT("CycleFormation"));
    if (CMappingOrder.Num() != 2 || F8MappingOrder.Num() != 2)
    {
        return false;
    }

    FDispatchRun CForward;
    FDispatchRun CReverse;
    const bool bCForwardCompleted = RunComponentDelegateSequence(
        *this,
        TEXT("C forward"),
        EKeys::C,
        CMappingOrder,
        false,
        true,
        CForward);
    const bool bCReverseCompleted = RunComponentDelegateSequence(
        *this,
        TEXT("C reverse"),
        EKeys::C,
        CMappingOrder,
        true,
        true,
        CReverse);
    TestTrue(TEXT("C forward component sequence completes"),
             bCForwardCompleted);
    TestTrue(TEXT("C reverse component sequence completes"),
             bCReverseCompleted);
    if (bCForwardCompleted && bCReverseCompleted)
    {
        if (!(CForward.Before == CReverse.Before))
        {
            AddError(FString::Printf(
                TEXT("C fixtures did not begin from equal semantic state: forward={%s} reverse={%s}"),
                *CForward.Before.Describe(),
                *CReverse.Before.Describe()));
        }
        if (!(CForward.After == CReverse.After))
        {
            AddError(FString::Printf(
                TEXT("C component order changed semantic state: forward={%s} reverse={%s}"),
                *CForward.After.Describe(),
                *CReverse.After.Describe()));
        }
        TestTrue(
            TEXT("C begins in the title context"),
            CForward.Before.bTitleVisible &&
                CForward.Before.bModalVisible &&
                CForward.Before.bScenarioPaused);
        TestTrue(
            TEXT("C continues the ready campaign into its briefing"),
            !CForward.After.bTitleVisible &&
                CForward.After.bBriefingVisible &&
                CForward.After.bModalVisible &&
                CForward.After.bScenarioPaused &&
                CForward.After.Operation ==
                    EEchoesOperationMode::CampaignPrologue);
        TestTrue(
            TEXT("C title dispatch does not leak into the Future Well choice"),
            CForward.After.FutureWellChoice ==
                CForward.Before.FutureWellChoice);
        TestTrue(
            TEXT("C title dispatch does not change battlefield formation"),
            CForward.After.Formation == CForward.Before.Formation);
        TestFalse(
            TEXT("C title dispatch does not open the online front door"),
            CForward.After.bOnlineFrontDoorVisible);
    }

    FDispatchRun F8Forward;
    FDispatchRun F8Reverse;
    const bool bF8ForwardCompleted = RunComponentDelegateSequence(
        *this,
        TEXT("F8 forward"),
        EKeys::F8,
        F8MappingOrder,
        false,
        false,
        F8Forward);
    const bool bF8ReverseCompleted = RunComponentDelegateSequence(
        *this,
        TEXT("F8 reverse"),
        EKeys::F8,
        F8MappingOrder,
        true,
        false,
        F8Reverse);
    TestTrue(TEXT("F8 forward component sequence completes"),
             bF8ForwardCompleted);
    TestTrue(TEXT("F8 reverse component sequence completes"),
             bF8ReverseCompleted);
    if (bF8ForwardCompleted && bF8ReverseCompleted)
    {
        if (!(F8Forward.Before == F8Reverse.Before))
        {
            AddError(FString::Printf(
                TEXT("F8 fixtures did not begin from equal semantic state: forward={%s} reverse={%s}"),
                *F8Forward.Before.Describe(),
                *F8Reverse.Before.Describe()));
        }
        if (!(F8Forward.After == F8Reverse.After))
        {
            AddError(FString::Printf(
                TEXT("F8 component order changed semantic state: forward={%s} reverse={%s}"),
                *F8Forward.After.Describe(),
                *F8Reverse.After.Describe()));
        }

        FPlayerDispatchState ExpectedF8State = F8Forward.Before;
        switch (ExpectedF8State.Formation)
        {
            case EEchoesFormationType::Box:
                ExpectedF8State.Formation = EEchoesFormationType::Line;
                break;
            case EEchoesFormationType::Line:
                ExpectedF8State.Formation = EEchoesFormationType::Wedge;
                break;
            case EEchoesFormationType::Wedge:
                ExpectedF8State.Formation = EEchoesFormationType::Box;
                break;
        }
        if (!(F8Forward.After == ExpectedF8State))
        {
            AddError(FString::Printf(
                TEXT("F8 battlefield dispatch changed state outside formation: expected={%s} actual={%s}"),
                *ExpectedF8State.Describe(),
                *F8Forward.After.Describe()));
        }
        TestFalse(
            TEXT("F8 battlefield dispatch does not open the title-only online front door"),
            F8Forward.After.bOnlineFrontDoorVisible);
    }

    return !HasAnyErrors();
}

#endif
