#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "Components/InputComponent.h"
#include "EchoesPlayerController.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

namespace
{
enum class EArrowDispatchContext : uint8
{
    Battlefield,
    TitleSkirmishSetup,
    TechnologyPanel
};

struct FArrowDispatchState final
{
    bool bTitleVisible = false;
    bool bBriefingVisible = false;
    bool bModalVisible = false;
    bool bSkirmishSetupVisible = false;
    bool bTechnologyPanelVisible = false;
    bool bPauseMenuVisible = false;
    bool bScenarioPaused = false;
    bool bKeyboardTargetingEnabled = false;
    FVector2D KeyboardTargetOffset = FVector2D::ZeroVector;
    int32 SkirmishFocusRow = 0;
    FEchoesSkirmishSetup SkirmishSetup;
    int32 TechnologyFocusedTier = 0;
    EEchoesFormationType Formation = EEchoesFormationType::Box;
    EEchoesOperationMode Operation = EEchoesOperationMode::Skirmish;
    uint64 SimulationTick = 0;
    uint64 SimulationChecksum = 0;
    int32 EntityCount = 0;

    [[nodiscard]] bool operator==(const FArrowDispatchState& Other) const
    {
        return bTitleVisible == Other.bTitleVisible &&
            bBriefingVisible == Other.bBriefingVisible &&
            bModalVisible == Other.bModalVisible &&
            bSkirmishSetupVisible == Other.bSkirmishSetupVisible &&
            bTechnologyPanelVisible == Other.bTechnologyPanelVisible &&
            bPauseMenuVisible == Other.bPauseMenuVisible &&
            bScenarioPaused == Other.bScenarioPaused &&
            bKeyboardTargetingEnabled == Other.bKeyboardTargetingEnabled &&
            KeyboardTargetOffset == Other.KeyboardTargetOffset &&
            SkirmishFocusRow == Other.SkirmishFocusRow &&
            SkirmishSetup == Other.SkirmishSetup &&
            TechnologyFocusedTier == Other.TechnologyFocusedTier &&
            Formation == Other.Formation &&
            Operation == Other.Operation &&
            SimulationTick == Other.SimulationTick &&
            SimulationChecksum == Other.SimulationChecksum &&
            EntityCount == Other.EntityCount;
    }

    [[nodiscard]] FString Describe() const
    {
        return FString::Printf(
            TEXT("title=%s briefing=%s modal=%s skirmishSetup=%s techPanel=%s ")
            TEXT("pause=%s paused=%s keyTarget=%s offset=(%.0f,%.0f) ")
            TEXT("skirmishRow=%d faction=%d/%d map=%d ai=%d resources=%d ")
            TEXT("tier=%d formation=%d operation=%d tick=%llu checksum=%llu ")
            TEXT("entities=%d"),
            bTitleVisible ? TEXT("true") : TEXT("false"),
            bBriefingVisible ? TEXT("true") : TEXT("false"),
            bModalVisible ? TEXT("true") : TEXT("false"),
            bSkirmishSetupVisible ? TEXT("true") : TEXT("false"),
            bTechnologyPanelVisible ? TEXT("true") : TEXT("false"),
            bPauseMenuVisible ? TEXT("true") : TEXT("false"),
            bScenarioPaused ? TEXT("true") : TEXT("false"),
            bKeyboardTargetingEnabled ? TEXT("true") : TEXT("false"),
            KeyboardTargetOffset.X,
            KeyboardTargetOffset.Y,
            SkirmishFocusRow,
            static_cast<int32>(SkirmishSetup.LocalFaction),
            static_cast<int32>(SkirmishSetup.OpponentFaction),
            static_cast<int32>(SkirmishSetup.MapPreset),
            static_cast<int32>(SkirmishSetup.AiPersonality),
            static_cast<int32>(SkirmishSetup.ResourceLevel),
            TechnologyFocusedTier,
            static_cast<int32>(Formation),
            static_cast<int32>(Operation),
            static_cast<unsigned long long>(SimulationTick),
            static_cast<unsigned long long>(SimulationChecksum),
            EntityCount);
    }
};

struct FArrowDispatchRun final
{
    FArrowDispatchState Before;
    FArrowDispatchState After;
    bool bViewportAvailable = false;
    FVector2D ViewportSize = FVector2D::ZeroVector;
};

[[nodiscard]] FArrowDispatchState CaptureArrowDispatchState(
    const AEchoesPlayerController& Controller,
    const UEchoesSimulationSubsystem& Bridge)
{
    FArrowDispatchState State;
    State.bTitleVisible = Controller.IsTitleScreenVisible();
    State.bBriefingVisible = Controller.IsMissionBriefingVisible();
    State.bModalVisible = Controller.IsModalOverlayVisible();
    State.bSkirmishSetupVisible = Controller.IsSkirmishSetupVisible();
    State.bTechnologyPanelVisible = Controller.IsTechnologyPanelVisible();
    State.bPauseMenuVisible = Controller.IsPauseMenuVisible();
    State.bScenarioPaused = Bridge.IsScenarioPaused();
    State.bKeyboardTargetingEnabled = Controller.IsKeyboardTargetingEnabled();
    State.KeyboardTargetOffset = Controller.GetKeyboardTargetOffset();
    State.SkirmishFocusRow = Controller.GetSkirmishSetupFocusRow();
    State.SkirmishSetup = Controller.GetPendingSkirmishSetup();
    State.TechnologyFocusedTier = Controller.GetTechnologyPanelFocusedTier();
    State.Formation = Controller.GetFormationType();
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

[[nodiscard]] TArray<FName> VerifySharedArrowMappings(
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

[[nodiscard]] bool RunArrowDelegateSequence(
    FAutomationTestBase& Test,
    const TCHAR* Scope,
    const FKey& PhysicalKey,
    const TArray<FName>& MappingOrder,
    const bool bReverseOrder,
    const EArrowDispatchContext Context,
    FArrowDispatchRun& OutRun)
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

    bool bContextReady = true;
    switch (Context)
    {
        case EArrowDispatchContext::Battlefield:
            if (Controller->IsModalOverlayVisible())
            {
                Test.AddError(FString::Printf(
                    TEXT("%s did not begin in the battlefield context."),
                    Scope));
                bContextReady = false;
            }
            break;
        case EArrowDispatchContext::TitleSkirmishSetup:
            Controller->PresentTitleScreen();
            if (!Controller->IsTitleScreenVisible() ||
                !Controller->IsSkirmishSetupVisible())
            {
                Test.AddError(FString::Printf(
                    TEXT("%s did not enter the title skirmish-setup context."),
                    Scope));
                bContextReady = false;
            }
            break;
        case EArrowDispatchContext::TechnologyPanel:
            Controller->ToggleTechnologyPanel();
            if (!Controller->IsTechnologyPanelVisible() ||
                !Controller->IsModalOverlayVisible())
            {
                Test.AddError(FString::Printf(
                    TEXT("%s did not open the technology panel context."),
                    Scope));
                bContextReady = false;
            }
            break;
    }

    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    Controller->GetViewportSize(ViewportWidth, ViewportHeight);
    OutRun.bViewportAvailable = ViewportWidth > 0 && ViewportHeight > 0;
    OutRun.ViewportSize = FVector2D(
        static_cast<float>(ViewportWidth),
        static_cast<float>(ViewportHeight));

    OutRun.Before = CaptureArrowDispatchState(*Controller, *Bridge);
    bool bExecutedEveryDelegate = bContextReady;
    if (bContextReady)
    {
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
    }
    OutRun.After = CaptureArrowDispatchState(*Controller, *Bridge);

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(&Test);
    return bExecutedEveryDelegate && !WorldWrapper.HasFailed();
}

/**
 * Mirrors the clamped reticle-nudge arithmetic so the battlefield expectation
 * is exact whenever the test environment does provide a viewport.
 */
[[nodiscard]] FArrowDispatchState ExpectedBattlefieldState(
    const FArrowDispatchRun& Run,
    const FVector2D& Direction)
{
    FArrowDispatchState Expected = Run.Before;
    if (!Run.bViewportAvailable)
    {
        return Expected;
    }
    constexpr float StepPixels = 64.0f;
    constexpr float EdgeMarginPixels = 32.0f;
    Expected.bKeyboardTargetingEnabled = true;
    FVector2D Offset =
        Run.Before.KeyboardTargetOffset + Direction * StepPixels;
    Offset.X = FMath::Clamp(
        Offset.X,
        -(Run.ViewportSize.X * 0.5f - EdgeMarginPixels),
        Run.ViewportSize.X * 0.5f - EdgeMarginPixels);
    Offset.Y = FMath::Clamp(
        Offset.Y,
        -(Run.ViewportSize.Y * 0.5f - EdgeMarginPixels),
        Run.ViewportSize.Y * 0.5f - EdgeMarginPixels);
    Expected.KeyboardTargetOffset = Offset;
    return Expected;
}

struct FArrowKeyCase final
{
    const TCHAR* Label = TEXT("");
    FKey Key;
    FName FirstAction;
    FName SecondAction;
    FVector2D BattlefieldDirection = FVector2D::ZeroVector;
    int32 TitleRowDelta = 0;
    int32 TitleFactionDirection = 0;
    int32 TechnologyTierDelta = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesArrowKeyCollisionTest,
    "Echoes.Runtime.Controls.SharedArrowDispatch",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesArrowKeyCollisionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    AddInfo(
        TEXT("Scope: this test executes real controller-bound InputComponent ")
        TEXT("delegates directly for the shared arrow keys. It does not call ")
        TEXT("UPlayerInput::ProcessInputStack and does not claim end-to-end ")
        TEXT("physical-key dispatch. The gamepad D-pad mappings live on ")
        TEXT("different physical keys and are outside this bounded claim. ")
        TEXT("When the headless test world provides no viewport, the ")
        TEXT("battlefield reticle nudge is exercised as its guarded no-op ")
        TEXT("branch; the rendered reticle remains a later physical-input ")
        TEXT("gate."));

    const UInputSettings* InputSettings = GetDefault<UInputSettings>();
    if (!TestNotNull(TEXT("Input settings are available"), InputSettings))
    {
        return false;
    }

    TArray<FArrowKeyCase> Cases;
    {
        FArrowKeyCase Up;
        Up.Label = TEXT("Up");
        Up.Key = EKeys::Up;
        Up.FirstAction = TEXT("TechnologyFocusPrevious");
        Up.SecondAction = TEXT("SkirmishFocusPrevious");
        Up.BattlefieldDirection = FVector2D(0.0f, -1.0f);
        Up.TitleRowDelta = -1;
        Up.TechnologyTierDelta = -1;
        Cases.Add(Up);

        FArrowKeyCase Down;
        Down.Label = TEXT("Down");
        Down.Key = EKeys::Down;
        Down.FirstAction = TEXT("TechnologyFocusNext");
        Down.SecondAction = TEXT("SkirmishFocusNext");
        Down.BattlefieldDirection = FVector2D(0.0f, 1.0f);
        Down.TitleRowDelta = 1;
        Down.TechnologyTierDelta = 1;
        Cases.Add(Down);

        FArrowKeyCase Left;
        Left.Label = TEXT("Left");
        Left.Key = EKeys::Left;
        Left.FirstAction = TEXT("SkirmishValuePrevious");
        Left.SecondAction = TEXT("KeyboardTargetLeft");
        Left.BattlefieldDirection = FVector2D(-1.0f, 0.0f);
        Left.TitleFactionDirection = -1;
        Cases.Add(Left);

        FArrowKeyCase Right;
        Right.Label = TEXT("Right");
        Right.Key = EKeys::Right;
        Right.FirstAction = TEXT("SkirmishValueNext");
        Right.SecondAction = TEXT("KeyboardTargetRight");
        Right.BattlefieldDirection = FVector2D(1.0f, 0.0f);
        Right.TitleFactionDirection = 1;
        Cases.Add(Right);
    }

    bool bReportedViewportBranch = false;
    for (const FArrowKeyCase& Case : Cases)
    {
        const TArray<FName> MappingOrder = VerifySharedArrowMappings(
            *this,
            *InputSettings,
            Case.Key,
            Case.FirstAction,
            Case.SecondAction);
        if (MappingOrder.Num() != 2)
        {
            continue;
        }

        struct FContextPlan final
        {
            EArrowDispatchContext Context =
                EArrowDispatchContext::Battlefield;
            const TCHAR* Name = TEXT("");
        };
        const FContextPlan Contexts[] = {
            {EArrowDispatchContext::Battlefield, TEXT("battlefield")},
            {EArrowDispatchContext::TitleSkirmishSetup,
             TEXT("title skirmish setup")},
            {EArrowDispatchContext::TechnologyPanel,
             TEXT("technology panel")},
        };
        for (const FContextPlan& Plan : Contexts)
        {
            const FString ForwardScope = FString::Printf(
                TEXT("%s forward in %s"), Case.Label, Plan.Name);
            const FString ReverseScope = FString::Printf(
                TEXT("%s reverse in %s"), Case.Label, Plan.Name);
            FArrowDispatchRun Forward;
            FArrowDispatchRun Reverse;
            const bool bForwardCompleted = RunArrowDelegateSequence(
                *this,
                *ForwardScope,
                Case.Key,
                MappingOrder,
                false,
                Plan.Context,
                Forward);
            const bool bReverseCompleted = RunArrowDelegateSequence(
                *this,
                *ReverseScope,
                Case.Key,
                MappingOrder,
                true,
                Plan.Context,
                Reverse);
            TestTrue(
                *FString::Printf(
                    TEXT("%s component sequence completes"), *ForwardScope),
                bForwardCompleted);
            TestTrue(
                *FString::Printf(
                    TEXT("%s component sequence completes"), *ReverseScope),
                bReverseCompleted);
            if (!bForwardCompleted || !bReverseCompleted)
            {
                continue;
            }

            if (!(Forward.Before == Reverse.Before))
            {
                AddError(FString::Printf(
                    TEXT("%s fixtures did not begin from equal semantic ")
                    TEXT("state: forward={%s} reverse={%s}"),
                    *ForwardScope,
                    *Forward.Before.Describe(),
                    *Reverse.Before.Describe()));
                continue;
            }
            if (!(Forward.After == Reverse.After))
            {
                AddError(FString::Printf(
                    TEXT("%s component order changed semantic state: ")
                    TEXT("forward={%s} reverse={%s}"),
                    *ForwardScope,
                    *Forward.After.Describe(),
                    *Reverse.After.Describe()));
            }

            FArrowDispatchState Expected = Forward.Before;
            switch (Plan.Context)
            {
                case EArrowDispatchContext::Battlefield:
                {
                    TestFalse(
                        *FString::Printf(
                            TEXT("%s begins with no modal overlay"),
                            *ForwardScope),
                        Forward.Before.bModalVisible);
                    if (!bReportedViewportBranch)
                    {
                        bReportedViewportBranch = true;
                        AddInfo(FString::Printf(
                            TEXT("Battlefield reticle branch exercised: ")
                            TEXT("viewportAvailable=%s size=(%.0f,%.0f)"),
                            Forward.bViewportAvailable
                                ? TEXT("true")
                                : TEXT("false"),
                            Forward.ViewportSize.X,
                            Forward.ViewportSize.Y));
                    }
                    Expected = ExpectedBattlefieldState(
                        Forward,
                        Case.BattlefieldDirection);
                    break;
                }
                case EArrowDispatchContext::TitleSkirmishSetup:
                {
                    TestTrue(
                        *FString::Printf(
                            TEXT("%s begins modal in the skirmish setup"),
                            *ForwardScope),
                        Forward.Before.bTitleVisible &&
                            Forward.Before.bModalVisible &&
                            Forward.Before.bSkirmishSetupVisible);
                    TestEqual(
                        *FString::Printf(
                            TEXT("%s begins on skirmish row 0"),
                            *ForwardScope),
                        Forward.Before.SkirmishFocusRow,
                        0);
                    TestFalse(
                        *FString::Printf(
                            TEXT("%s begins with keyboard targeting off"),
                            *ForwardScope),
                        Forward.Before.bKeyboardTargetingEnabled);
                    if (Case.TitleRowDelta != 0)
                    {
                        Expected.SkirmishFocusRow =
                            (Forward.Before.SkirmishFocusRow +
                             Case.TitleRowDelta + 5) % 5;
                    }
                    else
                    {
                        Expected.SkirmishSetup =
                            FEchoesSkirmishSetupModel::WithNextFaction(
                                Forward.Before.SkirmishSetup,
                                true,
                                Case.TitleFactionDirection);
                    }
                    break;
                }
                case EArrowDispatchContext::TechnologyPanel:
                {
                    TestTrue(
                        *FString::Printf(
                            TEXT("%s begins modal in the technology panel"),
                            *ForwardScope),
                        Forward.Before.bTechnologyPanelVisible &&
                            Forward.Before.bModalVisible &&
                            !Forward.Before.bTitleVisible);
                    TestEqual(
                        *FString::Printf(
                            TEXT("%s begins on technology tier 0"),
                            *ForwardScope),
                        Forward.Before.TechnologyFocusedTier,
                        0);
                    Expected.TechnologyFocusedTier = FMath::Clamp(
                        Forward.Before.TechnologyFocusedTier +
                            Case.TechnologyTierDelta,
                        0,
                        1);
                    break;
                }
            }

            if (!(Forward.After == Expected))
            {
                AddError(FString::Printf(
                    TEXT("%s dispatch changed state outside its context: ")
                    TEXT("expected={%s} actual={%s}"),
                    *ForwardScope,
                    *Expected.Describe(),
                    *Forward.After.Describe()));
            }
        }
    }

    return !HasAnyErrors();
}

#endif
