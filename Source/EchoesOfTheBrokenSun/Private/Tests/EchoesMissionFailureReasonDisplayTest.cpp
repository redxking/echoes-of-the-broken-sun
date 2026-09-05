// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesPrologueMissionModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesMissionFailureReasonDisplayTest,
    "Echoes.Runtime.Campaign.FailureReasonDisplay",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesMissionFailureReasonDisplayTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
    UEchoesNarrativeSubsystem* Narrative =
        GameInstance != nullptr ? GameInstance->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr;

    if (!TestNotNull(TEXT("Narrative subsystem is present on GameInstance"), Narrative) ||
        !TestTrue(TEXT("Narrative pack loaded successfully"), Narrative->IsReady()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    constexpr EEchoesOperationMode CampaignOperations[] = {
        EEchoesOperationMode::CampaignPrologue,
        EEchoesOperationMode::CampaignSevenAccounts,
        EEchoesOperationMode::CampaignCityReserve,
        EEchoesOperationMode::CampaignUnburiedRoad,
        EEchoesOperationMode::CampaignTermsOfContinuance,
        EEchoesOperationMode::CampaignNamesWithoutBirths,
        EEchoesOperationMode::CampaignShapeOfSilence,
        EEchoesOperationMode::CampaignShapeBesideUs,
        EEchoesOperationMode::CampaignReserveAuthority,
        EEchoesOperationMode::CampaignChoirAtLumeReach,
        EEchoesOperationMode::CampaignNoNeutralLedger,
        EEchoesOperationMode::CampaignFutureThatWon,
        EEchoesOperationMode::CampaignAssemblyOfTheMissing,
        EEchoesOperationMode::CampaignSeveralVoicesOneCommand,
        EEchoesOperationMode::CampaignTheBrokenSun
    };

    // 1. Verify generic failure condition and retry advice for every campaign mission
    for (int32 i = 0; i < UE_ARRAY_COUNT(CampaignOperations); ++i)
    {
        const EEchoesOperationMode Op = CampaignOperations[i];
        const FString GenericCondition = Narrative->GetFailureCondition(Op, TEXT("generic"));
        TestFalse(
            *FString::Printf(TEXT("Operation %d has non-empty generic failure condition"), i + 1),
            GenericCondition.IsEmpty());

        const FString RetryGuidance = Narrative->GetRetryCopy(Op);
        TestFalse(
            *FString::Printf(TEXT("Operation %d has non-empty retry guidance"), i + 1),
            RetryGuidance.IsEmpty());
    }

    // 2. Verify specific authored failure reason codes for Prologue (M01)
    const FString CoreLost = Narrative->GetFailureCondition(
        EEchoesOperationMode::CampaignPrologue, TEXT("local_core_lost"));
    TestFalse(TEXT("Prologue local_core_lost condition is populated"), CoreLost.IsEmpty());

    const FString CarrierLost = Narrative->GetFailureCondition(
        EEchoesOperationMode::CampaignPrologue, TEXT("archive_carrier_lost"));
    TestFalse(TEXT("Prologue archive_carrier_lost condition is populated"), CarrierLost.IsEmpty());

    const FString WellLost = Narrative->GetFailureCondition(
        EEchoesOperationMode::CampaignPrologue, TEXT("future_well_lost"));
    TestFalse(TEXT("Prologue future_well_lost condition is populated"), WellLost.IsEmpty());

    // Verify distinctness: specific failure causes do not collapse into generic text
    TestNotEqual(TEXT("local_core_lost is distinct from archive_carrier_lost"),
                 CoreLost, CarrierLost);
    TestNotEqual(TEXT("local_core_lost is distinct from future_well_lost"),
                 CoreLost, WellLost);

    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
