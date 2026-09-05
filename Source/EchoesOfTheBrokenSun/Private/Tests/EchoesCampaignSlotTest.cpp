#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCampaignSlotTest,
    "Echoes.Runtime.Persistence.CampaignSlots",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

namespace
{
/** A valid one-record ledger, distinguished by its founding choice. */
FEchoesCampaignProgress MakeProgress(
    echoes::sim::FutureWellChoice Choice,
    uint64 Checksum)
{
    FEchoesCampaignProgress Progress;
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices = 0x07;
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
    Record.CompletionTick = 240;
    Record.FinalStateChecksum = Checksum;
    FString Feedback;
    Progress.AppendDecision(Record, Feedback);
    return Progress;
}
}

bool FEchoesCampaignSlotTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    using FStore = FEchoesCampaignProgressStore;
    FString Feedback;

    // --- Name validation fails closed --------------------------------------

    TestTrue(TEXT("A plain name is valid"),
             FStore::IsValidSlotName(TEXT("First Run")));
    TestTrue(TEXT("Hyphens and underscores are valid"),
             FStore::IsValidSlotName(TEXT("run_2-final")));
    TestFalse(TEXT("An empty name is invalid"),
              FStore::IsValidSlotName(TEXT("")));
    TestFalse(TEXT("A 33-character name is invalid"),
              FStore::IsValidSlotName(
                  FString::ChrN(33, TEXT('a'))));
    TestFalse(TEXT("Path separators are invalid"),
              FStore::IsValidSlotName(TEXT("../escape")));
    TestFalse(TEXT("Dots are invalid"),
              FStore::IsValidSlotName(TEXT("name.bin")));
    TestFalse(TEXT("Leading spaces are invalid"),
              FStore::IsValidSlotName(TEXT(" padded")));
    FEchoesCampaignProgress Rejected;
    TestFalse(TEXT("Saving under an invalid name is refused"),
              FStore::SaveSlot(TEXT("../escape"), Rejected, Feedback));
    TestTrue(TEXT("The refusal carries the stable reason"),
             Feedback.Contains(TEXT("[CAMPAIGN_SLOT_NAME_INVALID]")));

    // --- Create, list, load ------------------------------------------------

    const FEchoesCampaignProgress PreserveRun =
        MakeProgress(echoes::sim::FutureWellChoice::Preserve, 1111);
    TestTrue(TEXT("A named slot saves transactionally"),
             FStore::SaveSlot(TEXT("First Run"), PreserveRun, Feedback));
    const FEchoesCampaignProgress UnderscoreRun =
        MakeProgress(echoes::sim::FutureWellChoice::Reshape, 4444);
    TestTrue(TEXT("An underscore-bearing name saves independently"),
             FStore::SaveSlot(TEXT("First_Run"), UnderscoreRun, Feedback));
    TestTrue(
        TEXT("Spaces and underscores resolve to distinct escaped paths"),
        FStore::GetSlotPath(TEXT("First Run")) !=
            FStore::GetSlotPath(TEXT("First_Run")) &&
            FStore::GetSlotPath(TEXT("First Run")).EndsWith(
                TEXT("EchoesCampaignSlot_First~sRun.bin")) &&
            FStore::GetSlotPath(TEXT("First_Run")).EndsWith(
                TEXT("EchoesCampaignSlot_First~uRun.bin")));
    const FEchoesCampaignProgress HarvestRun =
        MakeProgress(echoes::sim::FutureWellChoice::Harvest, 2222);
    TestTrue(TEXT("A second slot saves independently"),
             FStore::SaveSlot(TEXT("Branch B"), HarvestRun, Feedback));

    TArray<FStore::FSlotSummary> Slots = FStore::ListSlots();
    TestEqual(TEXT("All independently named slots list"), Slots.Num(), 3);
    if (Slots.Num() == 3)
    {
        TestEqual(TEXT("Slots list in stable order"),
                  Slots[0].SlotName, FString(TEXT("Branch B")));
        TestEqual(TEXT("Decision counts decode from the containers"),
                  Slots[0].DecisionCount, 1);
        TestEqual(TEXT("The second listed slot is First Run"),
                  Slots[1].SlotName, FString(TEXT("First Run")));
        TestEqual(TEXT("Its decision count decodes too"),
                  Slots[1].DecisionCount, 1);
        TestEqual(TEXT("The escaped underscore slot remains distinct"),
                  Slots[2].SlotName, FString(TEXT("First_Run")));
    }

    FEchoesCampaignProgress Loaded;
    TestTrue(TEXT("A named slot loads"),
             FStore::LoadSlot(TEXT("First Run"), Loaded, Feedback));
    TestTrue(TEXT("The loaded ledger carries its own decision"),
             Loaded.Decisions.Num() == 1 &&
                 Loaded.Decisions[0].WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 Loaded.Decisions[0].FinalStateChecksum == 1111);
    TestTrue(TEXT("The underscore slot loads its own ledger"),
             FStore::LoadSlot(TEXT("First_Run"), Loaded, Feedback));
    TestTrue(TEXT("The underscore slot did not inherit the space slot"),
             Loaded.Decisions.Num() == 1 &&
                 Loaded.Decisions[0].WellChoice ==
                     echoes::sim::FutureWellChoice::Reshape &&
                 Loaded.Decisions[0].FinalStateChecksum == 4444);

    // --- Overwrite keeps a prior generation --------------------------------

    const FEchoesCampaignProgress ReshapeRun =
        MakeProgress(echoes::sim::FutureWellChoice::Reshape, 3333);
    TestTrue(TEXT("Overwriting a slot succeeds"),
             FStore::SaveSlot(TEXT("First Run"), ReshapeRun, Feedback));
    TestTrue(TEXT("The overwritten slot loads the new ledger"),
             FStore::LoadSlot(TEXT("First Run"), Loaded, Feedback));
    TestTrue(TEXT("The new ledger carries the overwrite"),
             Loaded.Decisions.Num() == 1 &&
                 Loaded.Decisions[0].WellChoice ==
                     echoes::sim::FutureWellChoice::Reshape &&
                 Loaded.Decisions[0].FinalStateChecksum == 3333);
    TestTrue(TEXT("The prior generation survives as backup"),
             IFileManager::Get().FileExists(
                 *(FStore::GetSlotPath(TEXT("First Run")) + TEXT(".bak"))));

    // --- Corruption falls back to the prior generation ---------------------

    {
        TArray<uint8> Garbage;
        Garbage.Init(0x5A, 64);
        TestTrue(TEXT("Corrupting the primary for the recovery case"),
                 FFileHelper::SaveArrayToFile(
                     Garbage, *FStore::GetSlotPath(TEXT("First Run"))));
    }
    TestTrue(TEXT("A corrupt primary recovers from the backup"),
             FStore::LoadSlot(TEXT("First Run"), Loaded, Feedback));
    TestTrue(TEXT("The recovered ledger is the prior generation"),
             Loaded.Decisions.Num() == 1 &&
                 Loaded.Decisions[0].WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 Loaded.Decisions[0].FinalStateChecksum == 1111);

    // --- Delete removes both generations ------------------------------------

    TestTrue(TEXT("Deleting a slot succeeds"),
             FStore::DeleteSlot(TEXT("First Run"), Feedback));
    TestFalse(TEXT("The primary is gone"),
              IFileManager::Get().FileExists(
                  *FStore::GetSlotPath(TEXT("First Run"))));
    TestFalse(TEXT("The backup is gone"),
              IFileManager::Get().FileExists(
                  *(FStore::GetSlotPath(TEXT("First Run")) + TEXT(".bak"))));
    TestFalse(TEXT("Loading a deleted slot fails with a stable reason"),
              FStore::LoadSlot(TEXT("First Run"), Loaded, Feedback));
    TestTrue(TEXT("The absence reason is stable"),
             Feedback.Contains(TEXT("[CAMPAIGN_SLOT_ABSENT]")));
    TestFalse(TEXT("Deleting an absent slot fails honestly"),
             FStore::DeleteSlot(TEXT("First Run"), Feedback));

    Slots = FStore::ListSlots();
    TestEqual(TEXT("The independent underscore slot survives deletion"),
              Slots.Num(), 2);

    // --- Legacy aliases are preserved without being attributed ------------

    const FString LegacyAliasPath = FPaths::Combine(
        FStore::GetSaveGameDirectory(),
        TEXT("EchoesCampaignSlot_Legacy_Run.bin"));
    TestTrue(TEXT("A synthetic legacy alias fixture is written in isolation"),
             FStore::SaveAtomic(LegacyAliasPath, PreserveRun, Feedback));
    TestFalse(TEXT("A legacy space name is not guessed during load"),
              FStore::LoadSlot(TEXT("Legacy Run"), Loaded, Feedback));
    TestTrue(TEXT("The legacy space refusal has a stable migration reason"),
             Feedback.Contains(TEXT("[CAMPAIGN_SLOT_LEGACY_ALIAS]")));
    TestFalse(TEXT("A legacy underscore name is not guessed during load"),
              FStore::LoadSlot(TEXT("Legacy_Run"), Loaded, Feedback));
    TestTrue(TEXT("The legacy underscore refusal has the same stable reason"),
             Feedback.Contains(TEXT("[CAMPAIGN_SLOT_LEGACY_ALIAS]")));
    TestFalse(TEXT("Legacy aliases are not deleted by a guessed name"),
              FStore::DeleteSlot(TEXT("Legacy Run"), Feedback));
    TestTrue(TEXT("The legacy alias remains preserved after delete refusal"),
             IFileManager::Get().FileExists(*LegacyAliasPath));
    Slots = FStore::ListSlots();
    TestEqual(TEXT("Legacy aliases are not listed as attributable journeys"),
              Slots.Num(), 2);
    IFileManager::Get().Delete(*LegacyAliasPath, false, true, true);
    IFileManager::Get().Delete(*(LegacyAliasPath + TEXT(".bak")), false, true, true);

    // --- An unreadable slot is not listed as loadable ----------------------

    {
        TArray<uint8> Garbage;
        Garbage.Init(0x33, 48);
        FFileHelper::SaveArrayToFile(
            Garbage, *FStore::GetSlotPath(TEXT("Broken")));
    }
    Slots = FStore::ListSlots();
    TestEqual(TEXT("A wholly corrupt slot fails closed out of the list"),
              Slots.Num(), 2);
    FStore::DeleteSlot(TEXT("Broken"), Feedback);
    FStore::DeleteSlot(TEXT("Branch B"), Feedback);
    FStore::DeleteSlot(TEXT("First_Run"), Feedback);

    return true;
}

#endif
