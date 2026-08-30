#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignJourneyModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCampaignJourneyTest,
    "Echoes.Runtime.Campaign.Journey",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCampaignJourneyTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    constexpr EEchoesOperationMode ExpectedOperations[] = {
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
        EEchoesOperationMode::CampaignTheBrokenSun};

    FEchoesCampaignProgress Progress;
    for (int32 Completed = 0;
         Completed < UE_ARRAY_COUNT(ExpectedOperations);
         ++Completed)
    {
        const FEchoesCampaignJourney Journey =
            FEchoesCampaignJourneyModel::Resolve(Progress);
        TestEqual(
            *FString::Printf(
                TEXT("An ordered %d-record ledger has a ready continuation"),
                Completed),
            Journey.State,
            EEchoesCampaignJourneyState::Ready);
        TestEqual(
            *FString::Printf(
                TEXT("An ordered %d-record ledger selects the exact next mission"),
                Completed),
            Journey.NextOperation,
            ExpectedOperations[Completed]);
        TestEqual(
            TEXT("The journey reports its exact completed mission count"),
            Journey.CompletedMissionCount,
            Completed);

        FEchoesCampaignDecisionRecord Record;
        Record.Mission = static_cast<EEchoesCampaignMissionId>(Completed + 1);
        Progress.Decisions.Add(Record);
    }

    const FEchoesCampaignJourney CompleteJourney =
        FEchoesCampaignJourneyModel::Resolve(Progress);
    TestEqual(
        TEXT("The exact fifteen-record ledger is terminal"),
        CompleteJourney.State,
        EEchoesCampaignJourneyState::Complete);
    TestEqual(
        TEXT("A completed campaign does not invent Mission 16"),
        CompleteJourney.NextOperation,
        EEchoesOperationMode::Skirmish);

    FEchoesCampaignProgress Reordered = Progress;
    Swap(Reordered.Decisions[4], Reordered.Decisions[5]);
    TestEqual(
        TEXT("A reordered ledger cannot drive campaign continuation"),
        FEchoesCampaignJourneyModel::Resolve(Reordered).State,
        EEchoesCampaignJourneyState::Unavailable);

    FEchoesCampaignProgress Duplicate;
    Duplicate.Decisions.Add(Progress.Decisions[0]);
    Duplicate.Decisions.Add(Progress.Decisions[0]);
    TestEqual(
        TEXT("A duplicate mission prefix cannot drive campaign continuation"),
        FEchoesCampaignJourneyModel::Resolve(Duplicate).State,
        EEchoesCampaignJourneyState::Unavailable);

    FEchoesCampaignProgress Oversized = Progress;
    Oversized.Decisions.Add(Progress.Decisions.Last());
    TestEqual(
        TEXT("A ledger beyond the authored campaign fails closed"),
        FEchoesCampaignJourneyModel::Resolve(Oversized).State,
        EEchoesCampaignJourneyState::Unavailable);

    return !HasAnyErrors();
}

#endif
