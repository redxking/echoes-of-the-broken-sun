#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesGameUserSettings.h"
#include "EchoesPlayerProfile.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
void RemoveProfileGenerations(const FString& Path)
{
    IFileManager::Get().Delete(*Path, false, true, true);
    IFileManager::Get().Delete(*(Path + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(Path + TEXT(".tmp")), false, true, true);
}

void WriteProfileU16(TArray<uint8>& Bytes, int32 Offset, uint16 Value)
{
    Bytes[Offset] = static_cast<uint8>(Value);
    Bytes[Offset + 1] = static_cast<uint8>(Value >> 8);
}

void WriteProfileU32(TArray<uint8>& Bytes, int32 Offset, uint32 Value)
{
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Bytes[Offset + Index] =
            static_cast<uint8>(Value >> (Index * 8));
    }
}

void RefreshProfileChecksum(TArray<uint8>& Bytes)
{
    const int32 ChecksumOffset = Bytes.Num() - 4;
    WriteProfileU32(
        Bytes,
        ChecksumOffset,
        FCrc::MemCrc32(Bytes.GetData(), ChecksumOffset));
}

FEchoesPlayerProfile MakeProfile(uint8 Slot, uint16 TutorialMask)
{
    FEchoesPlayerProfile Profile;
    Profile.ActiveJourneySlot = Slot;
    Profile.bOnboardingOffered = true;
    Profile.bTutorialOptOut = false;
    Profile.TutorialVerifiedMask = TutorialMask;
    Profile.Resolution = FIntPoint(2560, 1440);
    Profile.WindowMode = EWindowMode::WindowedFullscreen;
    Profile.HudScale = 1.25f;
    Profile.bHighContrastHud = true;
    Profile.bReducedMotion = true;
    Profile.bReducedFlashing = true;
    Profile.bEdgePan = false;
    Profile.CameraPanSpeedScale = 1.5f;
    Profile.CameraZoomScale = 0.75f;
    Profile.EffectsVolume = 0.80f;
    Profile.bReducedDynamicRange = true;
    Profile.MasterVolume = 0.90f;
    Profile.MusicVolume = 0.40f;
    Profile.DialogueVolume = 0.70f;
    Profile.InterfaceVolume = 0.60f;
    Profile.AmbienceVolume = 0.50f;
    return Profile;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPlayerProfileTest,
    "Echoes.Runtime.Persistence.PlayerProfile",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPlayerProfileTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FString ProfilePath = FEchoesPlayerProfileStore::GetDefaultPath();
    TestTrue(TEXT("The profile uses the isolated save directory"),
             TestSaveEnvironment.IsPathScoped(ProfilePath));
    TestEqual(TEXT("The production profile filename is fixed"),
              FPaths::GetCleanFilename(ProfilePath),
              FString(TEXT("Profile.sav")));
    TestTrue(TEXT("Profile storage is independent of campaign storage"),
             ProfilePath != FEchoesCampaignProgressStore::GetDefaultPath());
    RemoveProfileGenerations(ProfilePath);

    FString Feedback;
    bool bExists = true;
    FEchoesPlayerProfile MissingSentinel = MakeProfile(3, 0x0007);
    const FEchoesPlayerProfile MissingBefore = MissingSentinel;
    TestTrue(TEXT("A wholly absent profile is a valid first-run state"),
             FEchoesPlayerProfileStore::LoadWithBackup(
                 ProfilePath, MissingSentinel, bExists, Feedback));
    TestFalse(TEXT("A wholly absent profile reports OutExists false"), bExists);
    TestTrue(TEXT("An absent profile does not replace caller state"),
             MissingSentinel == MissingBefore);

    const FEchoesPlayerProfile First = MakeProfile(2, 0x001F);
    TestFalse(TEXT("Partial verified curriculum does not grant mastery"),
              First.IsTutorialMasteryComplete());
    TestTrue(TEXT("A full authoritative curriculum derives mastery"),
             MakeProfile(2, FEchoesPlayerProfile::AllTutorialLessonsMask)
                 .IsTutorialMasteryComplete());
    FEchoesPlayerProfile NonContiguous = First;
    NonContiguous.TutorialVerifiedMask = 0x0005;
    TestFalse(TEXT("A noncontiguous lesson mask cannot derive mastery"),
              NonContiguous.IsTutorialMasteryComplete());

    TestTrue(TEXT("The first player profile commits atomically"),
             FEchoesPlayerProfileStore::SaveAtomic(
                 ProfilePath, First, Feedback));
    TArray<uint8> FirstBytes;
    TestTrue(TEXT("The committed profile can be inspected"),
             FFileHelper::LoadFileToArray(FirstBytes, *ProfilePath));
    FEchoesPlayerProfile Loaded;
    TestTrue(TEXT("The committed player profile reloads"),
             FEchoesPlayerProfileStore::LoadWithBackup(
                 ProfilePath, Loaded, bExists, Feedback));
    TestTrue(TEXT("The primary profile is reported present"), bExists);
    TestTrue(TEXT("The binary round trip preserves every profile field"),
             Loaded == First);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    const TArray<uint8> CampaignSentinel = {
        0x43, 0x41, 0x4d, 0x50, 0x41, 0x49, 0x47, 0x4e};
    TestTrue(TEXT("A campaign sentinel is staged in isolated storage"),
             FFileHelper::SaveArrayToFile(CampaignSentinel, *CampaignPath));
    FEchoesPlayerProfile IsolationUpdate = First;
    IsolationUpdate.ActiveJourneySlot = 3;
    TestTrue(TEXT("Updating the profile succeeds beside campaign storage"),
             FEchoesPlayerProfileStore::SaveAtomic(
                 ProfilePath, IsolationUpdate, Feedback));
    TArray<uint8> CampaignAfterProfileSave;
    TestTrue(TEXT("Profile saving does not alter campaign bytes"),
             FFileHelper::LoadFileToArray(
                 CampaignAfterProfileSave, *CampaignPath) &&
                 CampaignAfterProfileSave == CampaignSentinel);
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    FEchoesPlayerProfile LoadedAfterCampaignDelete;
    TestTrue(TEXT("Deleting campaign data does not erase the profile"),
             FEchoesPlayerProfileStore::LoadWithBackup(
                 ProfilePath,
                 LoadedAfterCampaignDelete,
                 bExists,
                 Feedback) &&
                 bExists && LoadedAfterCampaignDelete == IsolationUpdate);

    // Primary corruption must recover the prior valid generation without
    // promoting, deleting, or otherwise mutating either file during load.
    TestTrue(TEXT("A corrupt primary can be staged"),
             FFileHelper::SaveStringToFile(
                 TEXT("corrupt profile"), *ProfilePath));
    FEchoesPlayerProfile Recovered;
    TestTrue(TEXT("A valid backup recovers a corrupt primary"),
             FEchoesPlayerProfileStore::LoadWithBackup(
                 ProfilePath, Recovered, bExists, Feedback));
    TestTrue(TEXT("Backup recovery is disclosed with the required player message"),
             Feedback.Contains(TEXT("SAVE CORRUPT: RESTORING BACKUP")));
    TestTrue(TEXT("Backup recovery returns the prior valid generation"),
             Recovered == First);

    const auto VerifyRejectedBytes =
        [this, &ProfilePath, &Feedback](
            const TCHAR* Label,
            const TArray<uint8>& RejectedBytes,
            const TCHAR* ExpectedReason)
    {
        RemoveProfileGenerations(ProfilePath);
        TestTrue(
            FString::Printf(TEXT("%s fixture is written"), Label),
            FFileHelper::SaveArrayToFile(RejectedBytes, *ProfilePath));
        FEchoesPlayerProfile Sentinel = MakeProfile(3, 0x0003);
        const FEchoesPlayerProfile Before = Sentinel;
        bool bRejectedExists = false;
        const bool bLoaded = FEchoesPlayerProfileStore::LoadWithBackup(
            ProfilePath, Sentinel, bRejectedExists, Feedback);
        TestFalse(
            FString::Printf(TEXT("%s is rejected"), Label), bLoaded);
        TestTrue(
            FString::Printf(TEXT("%s remains present"), Label),
            bRejectedExists);
        TestTrue(
            FString::Printf(TEXT("%s does not mutate output"), Label),
            Sentinel == Before);
        TestTrue(
            FString::Printf(TEXT("%s reports its reason"), Label),
            Feedback.Contains(ExpectedReason));
    };

    TArray<uint8> InvalidSlot = FirstBytes;
    InvalidSlot[12] = 4;
    RefreshProfileChecksum(InvalidSlot);
    VerifyRejectedBytes(TEXT("Invalid slot"), InvalidSlot,
                        TEXT("PROFILE_SLOT_INVALID"));

    TArray<uint8> InvalidMask = FirstBytes;
    WriteProfileU16(InvalidMask, 14, 0x0005);
    RefreshProfileChecksum(InvalidMask);
    VerifyRejectedBytes(TEXT("Noncontiguous tutorial mask"), InvalidMask,
                        TEXT("PROFILE_TUTORIAL_MASK_INVALID"));

    TArray<uint8> InvalidOnboarding = FirstBytes;
    InvalidOnboarding[13] &= static_cast<uint8>(~(1u << 0));
    InvalidOnboarding[13] |= 1u << 1;
    RefreshProfileChecksum(InvalidOnboarding);
    VerifyRejectedBytes(TEXT("Unconfirmed tutorial opt-out"), InvalidOnboarding,
                        TEXT("PROFILE_ONBOARDING_STATE_INVALID"));

    TArray<uint8> InvalidSetting = FirstBytes;
    WriteProfileU32(InvalidSetting, 25, 0x7FC00000u);
    RefreshProfileChecksum(InvalidSetting);
    VerifyRejectedBytes(TEXT("Nonfinite HUD setting"), InvalidSetting,
                        TEXT("PROFILE_PRESENTATION_SETTING_INVALID"));

    TArray<uint8> InvalidSchema = FirstBytes;
    WriteProfileU16(InvalidSchema, 8, 2);
    RefreshProfileChecksum(InvalidSchema);
    VerifyRejectedBytes(TEXT("Unsupported schema"), InvalidSchema,
                        TEXT("PROFILE_VERSION_UNSUPPORTED"));

    TArray<uint8> InvalidChecksum = FirstBytes;
    InvalidChecksum[12] ^= 0x01;
    VerifyRejectedBytes(TEXT("Checksum mismatch"), InvalidChecksum,
                        TEXT("PROFILE_CHECKSUM_MISMATCH"));

    TArray<uint8> Truncated = FirstBytes;
    Truncated.SetNum(Truncated.Num() - 3);
    VerifyRejectedBytes(TEXT("Truncated profile"), Truncated,
                        TEXT("PROFILE_TRUNCATED"));

    RemoveProfileGenerations(ProfilePath);
    TestTrue(TEXT("An invalid primary is staged for both-invalid containment"),
             FFileHelper::SaveStringToFile(TEXT("invalid primary"), *ProfilePath));
    TestTrue(TEXT("An invalid backup is staged for both-invalid containment"),
             FFileHelper::SaveStringToFile(
                 TEXT("invalid backup"), *(ProfilePath + TEXT(".bak"))));
    FEchoesPlayerProfile BothInvalidSentinel = MakeProfile(2, 0x0001);
    const FEchoesPlayerProfile BothInvalidBefore = BothInvalidSentinel;
    TestFalse(TEXT("Two invalid generations fail closed"),
              FEchoesPlayerProfileStore::LoadWithBackup(
                  ProfilePath,
                  BothInvalidSentinel,
                  bExists,
                  Feedback));
    TestTrue(TEXT("Two invalid generations do not mutate output"),
             BothInvalidSentinel == BothInvalidBefore);

    RemoveProfileGenerations(ProfilePath);
    TestTrue(TEXT("A valid primary is staged for commit-failure rollback"),
             FEchoesPlayerProfileStore::SaveAtomic(
                 ProfilePath, First, Feedback));
    FEchoesPlayerProfileStore::FailNextCommitForTesting();
    TestFalse(TEXT("Injected commit failure is surfaced"),
              FEchoesPlayerProfileStore::SaveAtomic(
                  ProfilePath, IsolationUpdate, Feedback));
    TestTrue(TEXT("Injected failure reports a preserved prior generation"),
             Feedback.Contains(TEXT("PROFILE_COMMIT_FAILED")));
    FEchoesPlayerProfile AfterFailedCommit;
    TestTrue(TEXT("A failed write preserves the valid primary"),
             FEchoesPlayerProfileStore::LoadWithBackup(
                 ProfilePath, AfterFailedCommit, bExists, Feedback) &&
                 AfterFailedCommit == First);
    TestFalse(TEXT("A failed write leaves no temporary profile"),
              IFileManager::Get().FileExists(*(ProfilePath + TEXT(".tmp"))));

    TArray<uint8> ValidPrimaryBeforeReset;
    TestTrue(TEXT("The valid primary is captured before reset refusal"),
             FFileHelper::LoadFileToArray(
                 ValidPrimaryBeforeReset, *ProfilePath));
    TestFalse(TEXT("Explicit reset refuses an existing valid profile"),
              FEchoesPlayerProfileStore::ResetPreservingInvalidGenerations(
                  ProfilePath,
                  FEchoesPlayerProfile{},
                  Feedback));
    TestTrue(TEXT("Valid-profile reset refusal reports the recovery path"),
             Feedback.Contains(
                 TEXT("PROFILE_RESET_REFUSED_VALID_GENERATION")));
    TArray<uint8> ValidPrimaryAfterResetRefusal;
    TestTrue(TEXT("Reset refusal preserves the valid primary byte for byte"),
             FFileHelper::LoadFileToArray(
                 ValidPrimaryAfterResetRefusal, *ProfilePath) &&
                 ValidPrimaryAfterResetRefusal == ValidPrimaryBeforeReset);

    const FString TransientValidPath = FPaths::Combine(
        TestSaveEnvironment.Directory,
        TEXT("ProfileTransientValid.sav"));
    const TArray<uint8> InvalidTransientPrimary = {0x11, 0x22, 0x33};
    TestTrue(TEXT("An invalid primary is staged beside a valid temporary generation"),
             FFileHelper::SaveArrayToFile(
                 InvalidTransientPrimary, *TransientValidPath));
    TestTrue(TEXT("A valid transient generation is staged"),
             FFileHelper::SaveArrayToFile(
                 FirstBytes, *(TransientValidPath + TEXT(".tmp"))));
    TestTrue(TEXT("Explicit reset archives an uncommitted valid temporary generation"),
              FEchoesPlayerProfileStore::ResetPreservingInvalidGenerations(
                  TransientValidPath, FEchoesPlayerProfile{}, Feedback));
    TArray<FString> TransientArchives;
    IFileManager::Get().FindFiles(TransientArchives, *(TransientValidPath + TEXT(".tmp.archived-*")), true, false);
    TestEqual(TEXT("Uncommitted temporary generation has one retained archive"), TransientArchives.Num(), 1);
    TArray<uint8> PreservedTransientValid;
    if (TransientArchives.Num() == 1)
        TestTrue(TEXT("Archived temporary bytes are preserved exactly"),
            FFileHelper::LoadFileToArray(PreservedTransientValid,
                *FPaths::Combine(FPaths::GetPath(TransientValidPath), TransientArchives[0])) && PreservedTransientValid == FirstBytes);

    RemoveProfileGenerations(ProfilePath);
    const TArray<uint8> InvalidResetPrimary = {
        0x50, 0x52, 0x49, 0x4d, 0x41, 0x52, 0x59};
    const TArray<uint8> InvalidResetBackup = {
        0x42, 0x41, 0x43, 0x4b, 0x55, 0x50};
    const TArray<uint8> InvalidResetTemporary = {
        0x54, 0x45, 0x4d, 0x50};
    TestTrue(TEXT("Invalid reset generations are staged"),
             FFileHelper::SaveArrayToFile(
                 InvalidResetPrimary, *ProfilePath) &&
                 FFileHelper::SaveArrayToFile(
                     InvalidResetBackup,
                     *(ProfilePath + TEXT(".bak"))) &&
                 FFileHelper::SaveArrayToFile(
                     InvalidResetTemporary,
                     *(ProfilePath + TEXT(".tmp"))));
    const FEchoesPlayerProfile ResetDefaults = MakeProfile(1, 0);
    TestTrue(TEXT("Confirmed reset commits defaults when every generation is invalid"),
             FEchoesPlayerProfileStore::ResetPreservingInvalidGenerations(
                 ProfilePath, ResetDefaults, Feedback));
    FEchoesPlayerProfile ResetLoaded;
    TestTrue(TEXT("Confirmed reset leaves a valid primary profile"),
             FEchoesPlayerProfileStore::LoadWithBackup(
                 ProfilePath, ResetLoaded, bExists, Feedback) &&
                 bExists && ResetLoaded == ResetDefaults);

    const auto VerifyUniqueInvalidArchive =
        [this](
            const FString& OriginalPath,
            const TArray<uint8>& ExpectedBytes)
    {
        TArray<FString> ArchiveNames;
        IFileManager::Get().FindFiles(
            ArchiveNames,
            *(OriginalPath + TEXT(".archived-*")),
            true,
            false);
        if (!TestEqual(
                TEXT("Exactly one unique invalid-generation archive exists"),
                ArchiveNames.Num(),
                1))
        {
            return;
        }
        TArray<uint8> ArchivedBytes;
        const FString ArchivePath = FPaths::Combine(
            FPaths::GetPath(OriginalPath),
            ArchiveNames[0]);
        TestTrue(TEXT("The invalid-generation archive preserves exact bytes"),
                 FFileHelper::LoadFileToArray(
                     ArchivedBytes, *ArchivePath) &&
                     ArchivedBytes == ExpectedBytes);
    };
    VerifyUniqueInvalidArchive(ProfilePath, InvalidResetPrimary);
    VerifyUniqueInvalidArchive(
        ProfilePath + TEXT(".bak"), InvalidResetBackup);
    VerifyUniqueInvalidArchive(
        ProfilePath + TEXT(".tmp"), InvalidResetTemporary);

    const FString ResetFailurePath = FPaths::Combine(
        TestSaveEnvironment.Directory,
        TEXT("ProfileResetFailure.sav"));
    const TArray<uint8> FailurePrimary = {0x01, 0x02, 0x03};
    const TArray<uint8> FailureBackup = {0x04, 0x05};
    const TArray<uint8> FailureTemporary = {0x06};
    TestTrue(TEXT("Rollback fixtures are staged"),
             FFileHelper::SaveArrayToFile(
                 FailurePrimary, *ResetFailurePath) &&
                 FFileHelper::SaveArrayToFile(
                     FailureBackup,
                     *(ResetFailurePath + TEXT(".bak"))) &&
                 FFileHelper::SaveArrayToFile(
                     FailureTemporary,
                     *(ResetFailurePath + TEXT(".tmp"))));
    FEchoesPlayerProfileStore::FailNextCommitForTesting();
    TestFalse(TEXT("A reset commit failure is surfaced"),
              FEchoesPlayerProfileStore::ResetPreservingInvalidGenerations(
                  ResetFailurePath, ResetDefaults, Feedback));
    TestTrue(TEXT("Reset commit failure reports successful byte restoration"),
             Feedback.Contains(TEXT("PROFILE_RESET_COMMIT_FAILED")));
    TArray<uint8> FailurePrimaryAfter;
    TArray<uint8> FailureBackupAfter;
    TArray<uint8> FailureTemporaryAfter;
    TestTrue(TEXT("Failed reset restores all original invalid generations exactly"),
             FFileHelper::LoadFileToArray(
                 FailurePrimaryAfter, *ResetFailurePath) &&
                 FailurePrimaryAfter == FailurePrimary &&
                 FFileHelper::LoadFileToArray(
                     FailureBackupAfter,
                     *(ResetFailurePath + TEXT(".bak"))) &&
                 FailureBackupAfter == FailureBackup &&
                 FFileHelper::LoadFileToArray(
                     FailureTemporaryAfter,
                     *(ResetFailurePath + TEXT(".tmp"))) &&
                 FailureTemporaryAfter == FailureTemporary);
    TArray<FString> FailureArchives;
    IFileManager::Get().FindFiles(
        FailureArchives,
        *(ResetFailurePath + TEXT("*.archived-*")),
        true,
        false);
    TestTrue(TEXT("Successful rollback leaves no detached invalid archive"),
             FailureArchives.IsEmpty());

    UEchoesGameUserSettings* SourceSettings =
        NewObject<UEchoesGameUserSettings>(GetTransientPackage());
    UEchoesGameUserSettings* TargetSettings =
        NewObject<UEchoesGameUserSettings>(GetTransientPackage());
    if (!TestNotNull(TEXT("Source settings can be constructed"), SourceSettings) ||
        !TestNotNull(TEXT("Target settings can be constructed"), TargetSettings))
    {
        return false;
    }
    SourceSettings->SetToDefaults();
    SourceSettings->SetScreenResolution(FIntPoint(1920, 1080));
    SourceSettings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
    SourceSettings->SetHudScale(1.4f);
    SourceSettings->SetHighContrastHudEnabled(true);
    SourceSettings->SetReducedMotionEnabled(true);
    SourceSettings->SetReducedFlashingEnabled(true);
    SourceSettings->SetEdgePanEnabled(false);
    SourceSettings->SetCameraPanSpeedScale(1.75f);
    SourceSettings->SetCameraZoomScale(0.60f);
    SourceSettings->SetEffectsVolume(0.30f);
    SourceSettings->SetReducedDynamicRangeEnabled(true);
    SourceSettings->SetMasterVolume(0.80f);
    SourceSettings->SetMusicVolume(0.20f);
    SourceSettings->SetDialogueVolume(0.70f);
    SourceSettings->SetInterfaceVolume(0.40f);
    SourceSettings->SetAmbienceVolume(0.50f);

    FEchoesPlayerProfile Captured;
    Captured.CaptureSettings(*SourceSettings);
    TargetSettings->SetToDefaults();
    TestTrue(TEXT("A valid captured snapshot applies to user settings"),
             Captured.ApplySettings(*TargetSettings, Feedback));
    FEchoesPlayerProfile Recaptured;
    Recaptured.CaptureSettings(*TargetSettings);
    TestTrue(TEXT("Capture and apply preserve the complete settings snapshot"),
             Recaptured == Captured);

    FEchoesPlayerProfile InvalidApply = Captured;
    InvalidApply.HudScale = 7.0f;
    FEchoesPlayerProfile SettingsBeforeRejectedApply;
    SettingsBeforeRejectedApply.CaptureSettings(*TargetSettings);
    TestFalse(TEXT("Invalid profile settings are refused before application"),
              InvalidApply.ApplySettings(*TargetSettings, Feedback));
    FEchoesPlayerProfile SettingsAfterRejectedApply;
    SettingsAfterRejectedApply.CaptureSettings(*TargetSettings);
    TestTrue(TEXT("Rejected application leaves settings unchanged"),
             SettingsAfterRejectedApply == SettingsBeforeRejectedApply);

    RemoveProfileGenerations(ProfilePath);
    return TestSaveEnvironment.Finish();
}

#endif
