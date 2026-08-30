#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
struct FPreservedCampaignFile final
{
    explicit FPreservedCampaignFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedCampaignFile()
    {
        IFileManager::Get().Delete(*Path, false, true, true);
        if (bExisted)
        {
            FFileHelper::SaveArrayToFile(Contents, *Path);
        }
    }

    FString Path;
    TArray<uint8> Contents;
    bool bExisted = false;
};

FEchoesCampaignDecisionRecord MakeDecision(
    echoes::sim::FutureWellChoice Choice,
    uint64 CompletionTick,
    uint64 FinalChecksum)
{
    FEchoesCampaignDecisionRecord Record;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices = 0x07;
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
    Record.CompletionTick = CompletionTick;
    Record.FinalStateChecksum = FinalChecksum;
    return Record;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCampaignProgressTest,
    "Echoes.Runtime.Persistence.CampaignProgress",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCampaignProgressTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesCampaignProgress Progress;
    FString Feedback;
    const FEchoesCampaignDecisionRecord Preserve = MakeDecision(
        echoes::sim::FutureWellChoice::Preserve,
        377,
        0x1234'5678'9abc'def0ULL);
    TestTrue(
        TEXT("The first authoritative mission consequence appends"),
        Progress.AppendDecision(Preserve, Feedback) ==
            EEchoesCampaignCommitStatus::Added);
    TestEqual(TEXT("Exactly one mission decision is retained"),
              Progress.Decisions.Num(), 1);

    FEchoesCampaignDecisionRecord PreserveReplay = Preserve;
    PreserveReplay.CompletionTick += 50;
    PreserveReplay.FinalStateChecksum += 1;
    TestTrue(
        TEXT("A same-choice replay is idempotent"),
        Progress.AppendDecision(PreserveReplay, Feedback) ==
            EEchoesCampaignCommitStatus::AlreadyRecorded);
    TestEqual(TEXT("Idempotent replay does not duplicate the mission"),
              Progress.Decisions.Num(), 1);
    TestEqual(TEXT("The first completion tick remains provenance"),
              Progress.Decisions[0].CompletionTick,
              Preserve.CompletionTick);

    const FEchoesCampaignDecisionRecord HarvestReplay = MakeDecision(
        echoes::sim::FutureWellChoice::Harvest,
        450,
        0x2222'3333'4444'5555ULL);
    TestTrue(
        TEXT("A replay cannot rewrite an irreversible campaign choice"),
        Progress.AppendDecision(HarvestReplay, Feedback) ==
            EEchoesCampaignCommitStatus::ReplayConflict);
    TestTrue(TEXT("The original Preserve decision remains effective"),
             Progress.Decisions[0].WellChoice ==
                 echoes::sim::FutureWellChoice::Preserve);

    TArray<uint8> Encoded;
    TestTrue(TEXT("The ledger encodes to a versioned binary record"),
             FEchoesCampaignProgressStore::Encode(
                 Progress,
                 Encoded,
                 Feedback));
    FEchoesCampaignProgress Decoded;
    TestTrue(TEXT("The encoded ledger decodes"),
             FEchoesCampaignProgressStore::Decode(
                 Encoded,
                 Decoded,
                 Feedback));
    TestTrue(TEXT("The binary round trip preserves every decision field"),
             Decoded.Decisions == Progress.Decisions);

    TArray<uint8> CorruptBytes = Encoded;
    CorruptBytes[12] ^= 0x40;
    TestFalse(TEXT("Checksum validation rejects a modified record"),
              FEchoesCampaignProgressStore::Decode(
                  CorruptBytes,
                  Decoded,
                  Feedback));
    TestTrue(TEXT("Corruption reports an integrity failure"),
             Feedback.Contains(TEXT("CHECKSUM_MISMATCH")));

    FEchoesCampaignProgress InvalidProgress;
    FEchoesCampaignDecisionRecord Unverified = Preserve;
    Unverified.VerifiedFacts = 0;
    InvalidProgress.Decisions.Add(Unverified);
    TestFalse(TEXT("Unverified mission facts cannot be serialized"),
              FEchoesCampaignProgressStore::Encode(
                  InvalidProgress,
                  Encoded,
                  Feedback));
    TestTrue(TEXT("Rejected records identify the missing completion proof"),
             Feedback.Contains(TEXT("UNVERIFIED_COMPLETION")));

    const FString TestPath = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Automation"),
        TEXT("EchoesCampaignProgressTest.bin"));
    FPreservedCampaignFile Primary(TestPath);
    FPreservedCampaignFile Backup(TestPath + TEXT(".bak"));
    FPreservedCampaignFile Temporary(TestPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*TestPath, false, true, true);
    IFileManager::Get().Delete(*(TestPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(TestPath + TEXT(".tmp")), false, true, true);

    FEchoesCampaignProgress MissingProgress;
    TestTrue(TEXT("An absent ledger starts a new empty campaign"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 TestPath,
                 MissingProgress,
                 Feedback));
    TestTrue(TEXT("The new campaign contains no fabricated decisions"),
             MissingProgress.Decisions.IsEmpty());

    TestTrue(TEXT("A first campaign generation commits atomically"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 TestPath,
                 Progress,
                 Feedback));
    FEchoesCampaignProgress Loaded;
    TestTrue(TEXT("The primary campaign generation reloads"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 TestPath,
                 Loaded,
                 Feedback));
    TestTrue(TEXT("Primary reload matches the committed state"),
             Loaded.Decisions == Progress.Decisions);

    FEchoesCampaignProgress SecondGeneration = Progress;
    SecondGeneration.Decisions[0].CompletionTick += 100;
    SecondGeneration.Decisions[0].FinalStateChecksum += 100;
    TestTrue(TEXT("A second generation retains a validated backup"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 TestPath,
                 SecondGeneration,
                 Feedback));
    TestTrue(TEXT("The prior generation exists after replacement"),
             IFileManager::Get().FileExists(*(TestPath + TEXT(".bak"))));
    FEchoesCampaignProgress ExactBackup;
    TestTrue(TEXT("An explicit generation load validates only the named backup"),
             FEchoesCampaignProgressStore::LoadGeneration(
                 TestPath + TEXT(".bak"),
                 ExactBackup,
                 Feedback));
    TestTrue(TEXT("The exact backup is the first committed generation"),
             ExactBackup.Decisions == Progress.Decisions);
    TestFalse(TEXT("An unavailable exact generation does not fall back"),
              FEchoesCampaignProgressStore::LoadGeneration(
                  TestPath + TEXT(".missing"),
                  ExactBackup,
                  Feedback));
    TestTrue(TEXT("The unavailable generation reports its exact boundary"),
             Feedback.Contains(TEXT("GENERATION_UNAVAILABLE")));

    TestTrue(TEXT("The primary can be replaced with controlled corruption"),
             FFileHelper::SaveStringToFile(TEXT("corrupt"), *TestPath));
    TestTrue(TEXT("A corrupt primary recovers the prior generation"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 TestPath,
                 Loaded,
                 Feedback));
    TestTrue(TEXT("Backup recovery is disclosed"),
             Feedback.Contains(TEXT("backup recovered")));
    TestTrue(TEXT("Recovered state is the exact first generation"),
             Loaded.Decisions == Progress.Decisions);

    TestTrue(TEXT("Saving after fallback recovery commits a new valid primary"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 TestPath,
                 SecondGeneration,
                 Feedback));
    TestTrue(TEXT("A corrupt primary never displaces the last valid backup"),
             FEchoesCampaignProgressStore::LoadGeneration(
                 TestPath + TEXT(".bak"),
                 ExactBackup,
                 Feedback) &&
                 ExactBackup.Decisions == Progress.Decisions);

    TestTrue(TEXT("The replacement primary can be corrupted independently"),
             FFileHelper::SaveStringToFile(TEXT("corrupt again"), *TestPath));
    TestTrue(TEXT("The backup can also be replaced with corruption"),
             FFileHelper::SaveStringToFile(
                 TEXT("also corrupt"),
                 *(TestPath + TEXT(".bak"))));
    TestFalse(TEXT("An explicitly corrupt generation fails closed"),
              FEchoesCampaignProgressStore::LoadGeneration(
                  TestPath + TEXT(".bak"),
                  ExactBackup,
                  Feedback));
    TestTrue(TEXT("Exact-generation corruption is identified"),
             Feedback.Contains(TEXT("GENERATION_INVALID")));
    TestFalse(TEXT("Two corrupt generations fail closed"),
              FEchoesCampaignProgressStore::LoadWithBackup(
                  TestPath,
                  Loaded,
                  Feedback));
    TestTrue(TEXT("The closed failure identifies both invalid generations"),
             Feedback.Contains(TEXT("NO_VALID_LEDGER")));
    return !HasAnyErrors();
}

#endif
