#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

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

TArray<uint8> MakeFrozenLegacySchemaOneLedger()
{
    // Historically valid schema-one encoding corresponding to a Mission 01
    // Preserve record at simulation snapshot schema 22. The literal fixture
    // keeps the current encoder and CRC implementation from defining their
    // own compatibility evidence.
    static constexpr uint8 Bytes[] = {
        0x45, 0x43, 0x48, 0x4f, 0x43, 0x50, 0x47, 0x31,
        0x01, 0x00, 0x01, 0x00,
        0x01, 0x02, 0x07, 0x0f,
        0x16, 0x00, 0x00, 0x00,
        0x79, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
        0x62, 0xcf, 0x73, 0x0c};
    return TArray<uint8>(Bytes, UE_ARRAY_COUNT(Bytes));
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

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

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

    const TArray<uint8> LegacyEncoded =
        MakeFrozenLegacySchemaOneLedger();
    FEchoesCampaignProgress MigratedLegacy;
    TestTrue(TEXT("A schema-one campaign ledger migrates in memory"),
             FEchoesCampaignProgressStore::Decode(
                 LegacyEncoded,
                 MigratedLegacy,
                 Feedback));
    TestTrue(
        TEXT("Legacy migration preserves the decision and adds no ending"),
        MigratedLegacy.Decisions.Num() == 1 &&
            MigratedLegacy.Decisions[0].Mission ==
                EEchoesCampaignMissionId::WhatTheLedgerKeeps &&
            MigratedLegacy.Decisions[0].WellChoice == Preserve.WellChoice &&
            MigratedLegacy.Decisions[0].AvailableWellChoices == 0x07 &&
            MigratedLegacy.Decisions[0].VerifiedFacts == 0x0f &&
            MigratedLegacy.Decisions[0].FinalResolution ==
                EEchoesFinalResolution::None &&
            MigratedLegacy.Decisions[0].AvailableFinalResolutions == 0 &&
            MigratedLegacy.Decisions[0].FinalPlanKey == 0xFF &&
            MigratedLegacy.Decisions[0].SimulationSnapshotVersion == 22 &&
            MigratedLegacy.Decisions[0].CompletionTick == 377 &&
            MigratedLegacy.Decisions[0].FinalStateChecksum ==
                0x1234'5678'9abc'def0ULL);
    TArray<uint8> MigratedEncoding;
    TestTrue(TEXT("A migrated ledger re-encodes as schema two"),
             FEchoesCampaignProgressStore::Encode(
                 MigratedLegacy,
                 MigratedEncoding,
                 Feedback) &&
                 MigratedEncoding.Num() > 10 &&
                 MigratedEncoding[8] == 2 &&
                 MigratedEncoding[9] == 0);

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
        TestSaveEnvironment.Directory,
        TEXT("EchoesCampaignProgressTest.bin"));
    FPreservedCampaignFile Primary(TestPath);
    FPreservedCampaignFile Backup(TestPath + TEXT(".bak"));
    FPreservedCampaignFile Temporary(TestPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*TestPath, false, true, true);
    IFileManager::Get().Delete(*(TestPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(TestPath + TEXT(".tmp")), false, true, true);

    TestTrue(TEXT("The frozen schema-one primary is written exactly"),
             FFileHelper::SaveArrayToFile(LegacyEncoded, *TestPath));
    TArray<uint8> FrozenFixtureReadback;
    TestTrue(TEXT("The frozen schema-one fixture reads back byte for byte"),
             FFileHelper::LoadFileToArray(
                 FrozenFixtureReadback,
                 *TestPath) &&
                 FrozenFixtureReadback == LegacyEncoded);

    FEchoesCampaignProgress LoadedLegacy;
    TestTrue(TEXT("The schema-one primary loads through production recovery"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 TestPath,
                 LoadedLegacy,
                 Feedback));
    TestTrue(TEXT("The schema-one primary load is disclosed"),
             Feedback.Contains(TEXT("primary record loaded")));
    TestTrue(TEXT("The loaded schema-one decision is exact"),
             LoadedLegacy.Decisions == MigratedLegacy.Decisions);
    TArray<uint8> LegacyBytesAfterLoad;
    TestTrue(TEXT("Loading schema one does not rewrite the primary"),
             FFileHelper::LoadFileToArray(
                 LegacyBytesAfterLoad,
                 *TestPath) &&
                 LegacyBytesAfterLoad == LegacyEncoded);
    TestFalse(TEXT("Loading schema one does not create a backup"),
              IFileManager::Get().FileExists(*(TestPath + TEXT(".bak"))));
    TestFalse(TEXT("Loading schema one leaves no temporary file"),
              IFileManager::Get().FileExists(*(TestPath + TEXT(".tmp"))));

    TestTrue(TEXT("Saving loaded schema one promotes the current schema"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 TestPath,
                 LoadedLegacy,
                 Feedback));
    TArray<uint8> UpgradedPrimaryBytes;
    TArray<uint8> RetainedLegacyBytes;
    TestTrue(TEXT("The promoted primary is schema two"),
             FFileHelper::LoadFileToArray(
                 UpgradedPrimaryBytes,
                 *TestPath) &&
                 UpgradedPrimaryBytes.Num() > 10 &&
                 UpgradedPrimaryBytes[8] == 2 &&
                 UpgradedPrimaryBytes[9] == 0);
    TestTrue(TEXT("Promotion retains the exact schema-one generation"),
             FFileHelper::LoadFileToArray(
                 RetainedLegacyBytes,
                 *(TestPath + TEXT(".bak"))) &&
                 RetainedLegacyBytes == LegacyEncoded);
    TestFalse(TEXT("Schema promotion leaves no temporary file"),
              IFileManager::Get().FileExists(*(TestPath + TEXT(".tmp"))));

    FEchoesCampaignProgress UpgradedPrimary;
    FEchoesCampaignProgress RetainedLegacy;
    TestTrue(TEXT("The promoted primary reopens with the migrated decision"),
             FEchoesCampaignProgressStore::LoadGeneration(
                 TestPath,
                 UpgradedPrimary,
                 Feedback) &&
                 UpgradedPrimary.Decisions == MigratedLegacy.Decisions);
    TestTrue(TEXT("The exact schema-one backup remains readable"),
             FEchoesCampaignProgressStore::LoadGeneration(
                 TestPath + TEXT(".bak"),
                 RetainedLegacy,
                 Feedback) &&
                 RetainedLegacy.Decisions == MigratedLegacy.Decisions);

    if (!TestTrue(
            TEXT("The promoted primary exposes a payload byte for corruption"),
            UpgradedPrimaryBytes.IsValidIndex(12)))
    {
        return false;
    }
    TArray<uint8> CorruptUpgradedPrimary = UpgradedPrimaryBytes;
    CorruptUpgradedPrimary[12] ^= 0x40;
    TestTrue(TEXT("The promoted primary accepts a controlled payload mutation"),
             FFileHelper::SaveArrayToFile(
                 CorruptUpgradedPrimary,
                 *TestPath));
    FEchoesCampaignProgress RejectedCorruptPrimary;
    TestFalse(TEXT("The CRC-damaged promoted primary fails exact loading"),
              FEchoesCampaignProgressStore::LoadGeneration(
                  TestPath,
                  RejectedCorruptPrimary,
                  Feedback));
    TestTrue(TEXT("Exact loading identifies the CRC failure"),
             Feedback.Contains(TEXT("GENERATION_INVALID")) &&
                 Feedback.Contains(TEXT("CHECKSUM_MISMATCH")));

    FEchoesCampaignProgress RecoveredLegacy;
    TestTrue(TEXT("A damaged promoted primary recovers schema one"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 TestPath,
                 RecoveredLegacy,
                 Feedback));
    TestTrue(TEXT("Schema-one fallback recovery is disclosed"),
             Feedback.Contains(TEXT("backup recovered")));
    TestTrue(TEXT("Fallback returns the exact migrated decision"),
             RecoveredLegacy.Decisions == MigratedLegacy.Decisions);
    TArray<uint8> PrimaryBytesAfterFallback;
    TArray<uint8> BackupBytesAfterFallback;
    TestTrue(TEXT("Fallback does not repair the damaged primary"),
             FFileHelper::LoadFileToArray(
                 PrimaryBytesAfterFallback,
                 *TestPath) &&
                 PrimaryBytesAfterFallback == CorruptUpgradedPrimary);
    TestTrue(TEXT("Fallback leaves the schema-one backup byte exact"),
             FFileHelper::LoadFileToArray(
                 BackupBytesAfterFallback,
                 *(TestPath + TEXT(".bak"))) &&
                 BackupBytesAfterFallback == LegacyEncoded);
    TestFalse(TEXT("Fallback recovery leaves no temporary file"),
              IFileManager::Get().FileExists(*(TestPath + TEXT(".tmp"))));

    IFileManager::Get().Delete(*TestPath, false, true, true);
    IFileManager::Get().Delete(*(TestPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(TestPath + TEXT(".tmp")), false, true, true);
    if (!TestTrue(
            TEXT("Migration fixture cleanup resets all campaign generations"),
            !IFileManager::Get().FileExists(*TestPath) &&
                !IFileManager::Get().FileExists(
                    *(TestPath + TEXT(".bak"))) &&
                !IFileManager::Get().FileExists(
                    *(TestPath + TEXT(".tmp")))))
    {
        return false;
    }

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

    FEchoesCampaignProgress ThirdGeneration = SecondGeneration;
    ThirdGeneration.Decisions[0].CompletionTick += 100;
    ThirdGeneration.Decisions[0].FinalStateChecksum += 100;
    FEchoesCampaignProgressStore::FailNextBackupRotationForTesting();
    TestFalse(
        TEXT("A forced atomic backup-rotation fault rejects the commit"),
        FEchoesCampaignProgressStore::SaveAtomic(
            TestPath,
            ThirdGeneration,
            Feedback));
    TestTrue(
        TEXT("Backup-rotation failure is identified"),
        Feedback.Contains(TEXT("CAMPAIGN_BACKUP_FAILED")));
    FEchoesCampaignProgress PrimaryAfterRotationFault;
    FEchoesCampaignProgress BackupAfterRotationFault;
    TestTrue(
        TEXT("A failed rotation leaves both campaign generations exact"),
        FEchoesCampaignProgressStore::LoadGeneration(
            TestPath,
            PrimaryAfterRotationFault,
            Feedback) &&
            PrimaryAfterRotationFault.Decisions ==
                SecondGeneration.Decisions &&
            FEchoesCampaignProgressStore::LoadGeneration(
                TestPath + TEXT(".bak"),
                BackupAfterRotationFault,
                Feedback) &&
            BackupAfterRotationFault.Decisions == Progress.Decisions);

    FEchoesCampaignProgressStore::FailNextCommitForTesting();
    TestFalse(
        TEXT("A forced primary-commit fault rejects the replacement"),
        FEchoesCampaignProgressStore::SaveAtomic(
            TestPath,
            ThirdGeneration,
            Feedback));
    TestTrue(
        TEXT("Primary-commit failure reports complete generation rollback"),
        Feedback.Contains(TEXT("CAMPAIGN_COMMIT_FAILED")));
    FEchoesCampaignProgress PrimaryAfterCommitFault;
    FEchoesCampaignProgress BackupAfterCommitFault;
    TestTrue(
        TEXT("Commit failure restores both the prior primary and backup"),
        FEchoesCampaignProgressStore::LoadGeneration(
            TestPath,
            PrimaryAfterCommitFault,
            Feedback) &&
            PrimaryAfterCommitFault.Decisions ==
                SecondGeneration.Decisions &&
            FEchoesCampaignProgressStore::LoadGeneration(
                TestPath + TEXT(".bak"),
                BackupAfterCommitFault,
                Feedback) &&
            BackupAfterCommitFault.Decisions == Progress.Decisions);

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
