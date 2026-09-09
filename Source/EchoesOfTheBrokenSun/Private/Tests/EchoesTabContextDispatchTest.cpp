#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

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
[[nodiscard]] const FInputActionBinding* FindPressedControllerBinding(
    FAutomationTestBase& Test,
    UInputComponent& InputComponent,
    AEchoesPlayerController& Controller,
    const FName ActionName)
{
    const FInputActionBinding* Result = nullptr;
    int32 Count = 0;
    for (int32 Index = 0; Index < InputComponent.GetNumActionBindings(); ++Index)
    {
        const FInputActionBinding& Binding = InputComponent.GetActionBinding(Index);
        if (Binding.GetActionName() == ActionName && Binding.KeyEvent == IE_Pressed &&
            Binding.ActionDelegate.IsBoundToObject(&Controller))
        {
            ++Count;
            Result = &Binding;
        }
    }
    Test.TestEqual(
        *FString::Printf(TEXT("Controller has one pressed %s binding"), *ActionName.ToString()),
        Count,
        1);
    return Count == 1 ? Result : nullptr;
}

struct FTabDispatchResult final
{
    bool bContextReady = false;
    TArray<uint32> SelectedEntityIds;
    bool bAllSelectionsOwnedOrEmpty = true;
};

[[nodiscard]] bool RunFieldTabDispatch(
    FAutomationTestBase& Test,
    bool bReverseOrder,
    FTabDispatchResult& OutResult)
{
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(&Test);
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (Bridge == nullptr || !Bridge->StartPrototypeScenario())
    {
        Test.AddError(TEXT("Tab dispatch fixture could not start a scenario."));
        WorldWrapper.ForwardErrorMessages(&Test);
        return false;
    }

    AEchoesPlayerController* Controller = World->SpawnActor<AEchoesPlayerController>();
    if (Controller == nullptr)
    {
        Test.AddError(TEXT("Tab dispatch fixture could not create a controller."));
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(&Test);
        return false;
    }
    Controller->InitInputSystem();
    if (Controller->InputComponent == nullptr)
    {
        Controller->SetupInputComponent();
    }
    UInputComponent* InputComponent = Controller->InputComponent;
    if (InputComponent == nullptr)
    {
        Test.AddError(TEXT("Tab dispatch fixture has no input component."));
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(&Test);
        return false;
    }

    OutResult.bContextReady = !Controller->IsModalOverlayVisible() &&
        !Controller->IsTitleScreenVisible() && !Controller->IsMissionBriefingVisible();
    if (!OutResult.bContextReady)
    {
        Test.AddError(TEXT("Tab dispatch fixture did not begin on the battlefield."));
    }

    const FInputActionBinding* Faction = FindPressedControllerBinding(
        Test, *InputComponent, *Controller, TEXT("CyclePlayableFaction"));
    const FInputActionBinding* Next = FindPressedControllerBinding(
        Test, *InputComponent, *Controller, TEXT("CycleOwnedEntityNext"));
    if (OutResult.bContextReady && Faction != nullptr && Next != nullptr)
    {
        const FInputActionBinding* First = bReverseOrder ? Next : Faction;
        const FInputActionBinding* Second = bReverseOrder ? Faction : Next;
        First->ActionDelegate.Execute(EKeys::Tab);
        Second->ActionDelegate.Execute(EKeys::Tab);
        OutResult.SelectedEntityIds = Controller->GetSelectedEntityIds();
        for (const uint32 EntityId : OutResult.SelectedEntityIds)
        {
            const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
            OutResult.bAllSelectionsOwnedOrEmpty &= Entity != nullptr &&
                Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId;
        }
    }

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(&Test);
    return !WorldWrapper.HasFailed();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTabContextDispatchTest,
    "Echoes.Runtime.Controls.TabContextDispatch",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTabContextDispatchTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    AddInfo(
        TEXT("Scope: validates current action mappings and controller delegates for the shared Tab chord. ")
        TEXT("It invokes bound delegates in both orders; physical key-stack delivery remains an integration gate."));

    const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
    if (!TestNotNull(TEXT("Input settings are available"), InputSettings))
    {
        return false;
    }

    int32 TabMappingCount = 0;
    int32 PlainTabMappingCount = 0;
    bool bFactionOnPlainTab = false;
    bool bNextOnPlainTab = false;
    bool bPreviousOnShiftTab = false;
    // SPEC-CTL-014 reserves F1, F2 and F3 for the idle worker, production
    // structure and combat force selectors, so the campaign map cannot sit on F1.
    bool bCampaignMapOnReservedFreeKey = false;
    bool bCampaignMapClaimsReservedSelectorKey = false;
    for (const FInputActionKeyMapping& Mapping : InputSettings->GetActionMappings())
    {
        if (Mapping.Key == EKeys::Tab)
        {
            ++TabMappingCount;
            if (!Mapping.bShift && !Mapping.bCtrl && !Mapping.bAlt && !Mapping.bCmd)
            {
                ++PlainTabMappingCount;
                bFactionOnPlainTab |= Mapping.ActionName == TEXT("CyclePlayableFaction");
                bNextOnPlainTab |= Mapping.ActionName == TEXT("CycleOwnedEntityNext");
            }
            // Shift+Tab walks subgroups under SPEC-CTL-011. Backspace keeps the
            // separate owned-entity step, so the two commands are distinct.
            bPreviousOnShiftTab |= Mapping.bShift && !Mapping.bCtrl && !Mapping.bAlt &&
                !Mapping.bCmd &&
                Mapping.ActionName == TEXT("CycleSelectionSubgroupPrevious");
        }
        bCampaignMapOnReservedFreeKey |= Mapping.Key == EKeys::F12 && !Mapping.bShift &&
            !Mapping.bCtrl && !Mapping.bAlt && !Mapping.bCmd &&
            Mapping.ActionName == TEXT("ToggleCampaignOperationsMap");
        bCampaignMapClaimsReservedSelectorKey |=
            (Mapping.Key == EKeys::F1 || Mapping.Key == EKeys::F2 || Mapping.Key == EKeys::F3) &&
            Mapping.ActionName == TEXT("ToggleCampaignOperationsMap");
    }
    TestEqual(TEXT("Tab retains exactly its two plain context actions plus Shift-Tab"),
        TabMappingCount, 3);
    TestEqual(TEXT("Tab retains exactly two unmodified context actions"), PlainTabMappingCount, 2);
    TestTrue(TEXT("Plain Tab retains title faction cycling"), bFactionOnPlainTab);
    TestTrue(TEXT("Plain Tab retains field selection cycling"), bNextOnPlainTab);
    TestTrue(TEXT("Shift-Tab supplies reverse selection cycling"), bPreviousOnShiftTab);
    TestTrue(TEXT("Campaign operations reach a key of their own"), bCampaignMapOnReservedFreeKey);
    TestFalse(TEXT("Campaign operations leave the SPEC-CTL-014 selector keys free"),
        bCampaignMapClaimsReservedSelectorKey);

    FTabDispatchResult Forward;
    FTabDispatchResult Reverse;
    const bool bForwardRan = RunFieldTabDispatch(*this, false, Forward);
    const bool bReverseRan = RunFieldTabDispatch(*this, true, Reverse);
    TestTrue(TEXT("Forward shared-Tab delegate sequence runs"), bForwardRan);
    TestTrue(TEXT("Reverse shared-Tab delegate sequence runs"), bReverseRan);
    if (bForwardRan && bReverseRan)
    {
        TestTrue(TEXT("Forward sequence begins in the battlefield context"), Forward.bContextReady);
        TestTrue(TEXT("Reverse sequence begins in the battlefield context"), Reverse.bContextReady);
        TestTrue(TEXT("Forward shared Tab never selects a non-owned entity"), Forward.bAllSelectionsOwnedOrEmpty);
        TestTrue(TEXT("Reverse shared Tab never selects a non-owned entity"), Reverse.bAllSelectionsOwnedOrEmpty);
        TestEqual(TEXT("Shared Tab delegate order preserves the selected entity"),
            Forward.SelectedEntityIds, Reverse.SelectedEntityIds);
    }

    return !HasAnyErrors();
}

#endif
