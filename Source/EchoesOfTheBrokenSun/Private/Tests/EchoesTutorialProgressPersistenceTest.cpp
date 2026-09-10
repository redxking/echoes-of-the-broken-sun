#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesPlayerProfile.h"
#include "EchoesTutorialCurriculumModel.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// A uniquely named namespace, not an anonymous one. Unreal batches several
// .cpp files into one unity translation unit, and every anonymous namespace in
// that unit is the SAME namespace: two file-local helpers sharing a signature
// become a redefinition, and a sibling's parameter shadows a constant here.
// Each file still compiles alone, so nothing catches it until the batch.
namespace EchoesTutorialProgressPersistenceDetail
{
void RefreshChecksum(TArray<uint8>& Bytes)
{
    const int32 ChecksumOffset = Bytes.Num() - 4;
    const uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), ChecksumOffset);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Bytes[ChecksumOffset + Index] =
            static_cast<uint8>(Checksum >> (Index * 8));
    }
}

/**
 * Rewrite current-schema bytes as a genuine schema-two record. Schema two
 * predates the skipped and after-skip masks and the recorded contract width,
 * so it is five payload bytes shorter and declares that shorter length.
 */
void MakeSchemaTwoRecord(TArray<uint8>& Bytes)
{
    constexpr int32 SchemaTwoPayloadSize = 49;
    constexpr int32 ChecksumSize = 4;
    constexpr int32 AppendedSinceSchemaTwo = 5;
    Bytes.RemoveAt(
        Bytes.Num() - ChecksumSize - AppendedSinceSchemaTwo,
        AppendedSinceSchemaTwo);
    Bytes[8] = 2;
    Bytes[9] = 0;
    Bytes[10] = static_cast<uint8>(SchemaTwoPayloadSize);
    Bytes[11] = static_cast<uint8>(SchemaTwoPayloadSize >> 8);
    RefreshChecksum(Bytes);
}

/** The first lesson skipped, every later implemented lesson genuinely earned. */
FEchoesPlayerProfile SkippedThenEarnedProfile()
{
    FEchoesPlayerProfile Profile;
    Profile.ActiveJourneySlot = 1;
    Profile.bOnboardingOffered = true;
    Profile.TutorialSkippedMask = 1;
    Profile.TutorialSessionVerifiedMask = static_cast<uint16>(
        FEchoesPlayerProfile::AllTutorialLessonsMask & ~uint16(1));
    return Profile;
}
}  // namespace EchoesTutorialProgressPersistenceDetail

/**
 * Skipped lessons and lessons genuinely earned after a skip were controller
 * members that nothing serialized: skip lesson one, legitimately complete the
 * rest, quit, and every one of those completions was gone. This asserts both
 * records survive a reload, survive recovery from the backup generation, and
 * are never fabricated for an older record that never held them.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialProgressPersistenceTest,
    "Echoes.Runtime.Persistence.TutorialProgress",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialProgressPersistenceTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;
    FString Feedback;

    const FEchoesPlayerProfile Earned = EchoesTutorialProgressPersistenceDetail::SkippedThenEarnedProfile();
    TestEqual(
        TEXT("Skip plus later completions report the whole curriculum as reached"),
        Earned.GetTutorialProgressMask(),
        FEchoesPlayerProfile::AllTutorialLessonsMask);
    TestFalse(
        TEXT("Reaching every lesson through a skip is not mastery"),
        Earned.IsTutorialMasteryComplete());

    // --- The records survive a reload --------------------------------------
    const FString Path =
        FPaths::Combine(SaveEnvironment.Directory, TEXT("ProfileSkip.sav"));
    TestTrue(
        TEXT("A profile carrying skipped and after-skip lessons commits"),
        FEchoesPlayerProfileStore::SaveAtomic(Path, Earned, Feedback));
    FEchoesPlayerProfile Reloaded;
    bool bExists = false;
    TestTrue(
        TEXT("It reloads from disk"),
        FEchoesPlayerProfileStore::LoadWithBackup(
            Path, Reloaded, bExists, Feedback) && bExists);
    TestEqual(
        TEXT("The skip record survives the reload"),
        Reloaded.TutorialSkippedMask, Earned.TutorialSkippedMask);
    TestEqual(
        TEXT("Lessons earned after the skip survive the reload"),
        Reloaded.TutorialSessionVerifiedMask,
        Earned.TutorialSessionVerifiedMask);
    TestTrue(
        TEXT("Every other field round trips unchanged"), Reloaded == Earned);
    TestFalse(
        TEXT("A reload never promotes after-skip completions to mastery"),
        Reloaded.IsTutorialMasteryComplete() ||
            Reloaded.TutorialVerifiedMask != 0);

    // --- A corrupted primary still recovers the backup ---------------------
    FEchoesPlayerProfile Advanced = Earned;
    Advanced.ActiveJourneySlot = 2;
    TestTrue(
        TEXT("A second commit rotates the prior generation to backup"),
        FEchoesPlayerProfileStore::SaveAtomic(Path, Advanced, Feedback));
    TArray<uint8> Corrupt = {'E', 'C', 'H', 'O', 0, 0, 0, 0};
    TestTrue(
        TEXT("The primary generation is corrupted for the recovery check"),
        FFileHelper::SaveArrayToFile(Corrupt, *Path));
    FEchoesPlayerProfile Recovered;
    TestTrue(
        TEXT("A corrupted primary recovers from the backup generation"),
        FEchoesPlayerProfileStore::LoadWithBackup(
            Path, Recovered, bExists, Feedback) && bExists);
    TestEqual(
        TEXT("Backup recovery keeps the skip record"),
        Recovered.TutorialSkippedMask, Earned.TutorialSkippedMask);
    TestEqual(
        TEXT("Backup recovery keeps the lessons earned after the skip"),
        Recovered.TutorialSessionVerifiedMask,
        Earned.TutorialSessionVerifiedMask);

    // --- An older record is not given progress it never held ---------------
    const FString LegacyPath =
        FPaths::Combine(SaveEnvironment.Directory, TEXT("ProfileSchemaTwo.sav"));
    TestTrue(
        TEXT("A record is staged for the schema-two fixture"),
        FEchoesPlayerProfileStore::SaveAtomic(LegacyPath, Earned, Feedback));
    TArray<uint8> LegacyBytes;
    if (!TestTrue(
            TEXT("The staged record can be inspected"),
            FFileHelper::LoadFileToArray(LegacyBytes, *LegacyPath)))
    {
        return false;
    }
    EchoesTutorialProgressPersistenceDetail::MakeSchemaTwoRecord(LegacyBytes);
    TestTrue(
        TEXT("A checksum-valid schema-two fixture is written"),
        FFileHelper::SaveArrayToFile(LegacyBytes, *LegacyPath));
    FEchoesPlayerProfile Migrated;
    TestTrue(
        TEXT("A schema-two record still loads"),
        FEchoesPlayerProfileStore::LoadWithBackup(
            LegacyPath, Migrated, bExists, Feedback) && bExists);
    TestEqual(
        TEXT("A schema-two record reports no skipped lessons"),
        Migrated.TutorialSkippedMask, static_cast<uint16>(0));
    TestEqual(
        TEXT("A schema-two record reports no after-skip completions"),
        Migrated.TutorialSessionVerifiedMask, static_cast<uint16>(0));

    // --- Progress that no play sequence produces is refused ----------------
    if (EchoesTutorialLessonCount >= 3)
    {
        // Lesson one skipped and lesson three earned, with lesson two never
        // reached: a hole the player could not have walked through.
        FEchoesPlayerProfile Holed;
        Holed.ActiveJourneySlot = 1;
        Holed.bOnboardingOffered = true;
        Holed.TutorialSkippedMask = 1;
        Holed.TutorialSessionVerifiedMask = 4;
        const FString HoledPath = FPaths::Combine(
            SaveEnvironment.Directory, TEXT("ProfileHoledProgress.sav"));
        TestFalse(
            TEXT("Combined progress with a hole in it is refused"),
            FEchoesPlayerProfileStore::SaveAtomic(
                HoledPath, Holed, Feedback));
        TestTrue(
            TEXT("The refusal names the progress invariant"),
            Feedback.Contains(TEXT("PROFILE_TUTORIAL_PROGRESS_INVALID")));
    }

    // A lesson cannot be durable mastery and an after-skip completion at once.
    FEchoesPlayerProfile Doubled;
    Doubled.ActiveJourneySlot = 1;
    Doubled.bOnboardingOffered = true;
    Doubled.TutorialVerifiedMask = 1;
    Doubled.TutorialSessionVerifiedMask = 1;
    const FString DoubledPath = FPaths::Combine(
        SaveEnvironment.Directory, TEXT("ProfileDoubledLesson.sav"));
    TestFalse(
        TEXT("One lesson recorded in two progress classes is refused"),
        FEchoesPlayerProfileStore::SaveAtomic(DoubledPath, Doubled, Feedback));

    return true;
}

#endif
