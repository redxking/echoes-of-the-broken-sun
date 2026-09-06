#include "EchoesPlayerProfile.h"

#include "EchoesCampaignProgress.h"
#include "EchoesGameUserSettings.h"
#include "EchoesTutorialCurriculumModel.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace
{
constexpr uint8 ProfileMagic[] = {'E', 'C', 'H', 'O', 'P', 'R', 'F', '1'};
constexpr int32 ProfileHeaderSize = 12;
constexpr int32 ProfilePayloadSize = 49;
constexpr int32 ProfileChecksumSize = 4;
constexpr uint8 ProfileAllowedFlags = 0x7F;
constexpr int32 ProfileMinimumResolution = 320;
constexpr int32 ProfileMaximumResolution = 16384;

#if WITH_DEV_AUTOMATION_TESTS
bool bFailNextProfileCommitForTesting = false;
#endif

[[nodiscard]] bool AtomicReplaceProfileFile(
    const FString& Destination,
    const FString& Source)
{
#if PLATFORM_MAC
    return FPlatformFileManager::Get()
        .GetPlatformFile()
        .MoveFile(*Destination, *Source);
#else
    return IFileManager::Get().Move(
        *Destination, *Source, true, true, true, true);
#endif
}

void ProfileAppendU8(TArray<uint8>& Bytes, uint8 Value)
{
    Bytes.Add(Value);
}

void ProfileAppendU16(TArray<uint8>& Bytes, uint16 Value)
{
    Bytes.Add(static_cast<uint8>(Value));
    Bytes.Add(static_cast<uint8>(Value >> 8));
}

void ProfileAppendU32(TArray<uint8>& Bytes, uint32 Value)
{
    for (int32 Shift = 0; Shift < 32; Shift += 8)
    {
        Bytes.Add(static_cast<uint8>(Value >> Shift));
    }
}

void ProfileAppendI32(TArray<uint8>& Bytes, int32 Value)
{
    ProfileAppendU32(Bytes, static_cast<uint32>(Value));
}

void ProfileAppendFloat(TArray<uint8>& Bytes, float Value)
{
    uint32 Bits = 0;
    static_assert(sizeof(Bits) == sizeof(Value));
    FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
    ProfileAppendU32(Bytes, Bits);
}

[[nodiscard]] bool ProfileReadU8(
    const TArray<uint8>& Bytes,
    int32& Offset,
    uint8& OutValue)
{
    if (Offset < 0 || Offset >= Bytes.Num())
    {
        return false;
    }
    OutValue = Bytes[Offset++];
    return true;
}

[[nodiscard]] bool ProfileReadU16(
    const TArray<uint8>& Bytes,
    int32& Offset,
    uint16& OutValue)
{
    if (Offset < 0 || Offset > Bytes.Num() - 2)
    {
        return false;
    }
    OutValue = static_cast<uint16>(Bytes[Offset]) |
        (static_cast<uint16>(Bytes[Offset + 1]) << 8);
    Offset += 2;
    return true;
}

[[nodiscard]] bool ProfileReadU32(
    const TArray<uint8>& Bytes,
    int32& Offset,
    uint32& OutValue)
{
    if (Offset < 0 || Offset > Bytes.Num() - 4)
    {
        return false;
    }
    OutValue = static_cast<uint32>(Bytes[Offset]) |
        (static_cast<uint32>(Bytes[Offset + 1]) << 8) |
        (static_cast<uint32>(Bytes[Offset + 2]) << 16) |
        (static_cast<uint32>(Bytes[Offset + 3]) << 24);
    Offset += 4;
    return true;
}

[[nodiscard]] bool ProfileReadI32(
    const TArray<uint8>& Bytes,
    int32& Offset,
    int32& OutValue)
{
    uint32 Encoded = 0;
    if (!ProfileReadU32(Bytes, Offset, Encoded))
    {
        return false;
    }
    OutValue = static_cast<int32>(Encoded);
    return true;
}

[[nodiscard]] bool ProfileReadFloat(
    const TArray<uint8>& Bytes,
    int32& Offset,
    float& OutValue)
{
    uint32 Bits = 0;
    if (!ProfileReadU32(Bytes, Offset, Bits))
    {
        return false;
    }
    FMemory::Memcpy(&OutValue, &Bits, sizeof(OutValue));
    return true;
}

[[nodiscard]] bool IsOrderedTutorialMask(uint16 Mask)
{
    if ((Mask & ~FEchoesPlayerProfile::AllTutorialLessonsMask) != 0)
    {
        return false;
    }
    return (Mask & static_cast<uint16>(Mask + 1)) == 0;
}

[[nodiscard]] bool ValidateProfile(
    const FEchoesPlayerProfile& Profile,
    FString& OutError)
{
    if (Profile.ActiveJourneySlot < 1 || Profile.ActiveJourneySlot > 3)
    {
        OutError = TEXT("[PROFILE_SLOT_INVALID] ActiveJourneySlot must be 1, 2, or 3.");
        return false;
    }
    if (!IsOrderedTutorialMask(Profile.TutorialVerifiedMask))
    {
        OutError = TEXT("[PROFILE_TUTORIAL_MASK_INVALID] Verified tutorial lessons must be the contiguous authored prefix of the ten-lesson curriculum.");
        return false;
    }
    if (Profile.bTutorialOptOut && !Profile.bOnboardingOffered)
    {
        OutError = TEXT("[PROFILE_ONBOARDING_STATE_INVALID] Tutorial opt-out cannot precede the confirmed onboarding offer.");
        return false;
    }
    if (Profile.Resolution.X < ProfileMinimumResolution ||
        Profile.Resolution.Y < ProfileMinimumResolution ||
        Profile.Resolution.X > ProfileMaximumResolution ||
        Profile.Resolution.Y > ProfileMaximumResolution)
    {
        OutError = TEXT("[PROFILE_RESOLUTION_INVALID] Resolution dimensions must each be between 320 and 16384 pixels.");
        return false;
    }
    if (Profile.WindowMode < EWindowMode::Fullscreen ||
        Profile.WindowMode >= EWindowMode::NumWindowModes)
    {
        OutError = TEXT("[PROFILE_WINDOW_MODE_INVALID] Window mode is outside Unreal's supported persisted modes.");
        return false;
    }
    const auto IsBounded = [](float Value, float Minimum, float Maximum)
    {
        return FMath::IsFinite(Value) && Value >= Minimum && Value <= Maximum;
    };
    if (!IsBounded(Profile.HudScale, 0.80f, 1.50f) ||
        !IsBounded(Profile.CameraPanSpeedScale, 0.5f, 2.0f) ||
        !IsBounded(Profile.CameraZoomScale, 0.5f, 2.0f))
    {
        OutError = TEXT("[PROFILE_PRESENTATION_SETTING_INVALID] HUD or camera settings are outside their implemented bounds.");
        return false;
    }
    const float Volumes[] = {
        Profile.EffectsVolume,
        Profile.MasterVolume,
        Profile.MusicVolume,
        Profile.DialogueVolume,
        Profile.InterfaceVolume,
        Profile.AmbienceVolume};
    for (float Volume : Volumes)
    {
        if (!IsBounded(Volume, 0.0f, 1.0f))
        {
            OutError = TEXT("[PROFILE_AUDIO_SETTING_INVALID] Persisted audio volumes must be finite values from 0 through 1.");
            return false;
        }
    }
    OutError.Reset();
    return true;
}

[[nodiscard]] uint8 PackProfileFlags(const FEchoesPlayerProfile& Profile)
{
    return (Profile.bOnboardingOffered ? 1u << 0 : 0u) |
        (Profile.bTutorialOptOut ? 1u << 1 : 0u) |
        (Profile.bHighContrastHud ? 1u << 2 : 0u) |
        (Profile.bReducedMotion ? 1u << 3 : 0u) |
        (Profile.bReducedFlashing ? 1u << 4 : 0u) |
        (Profile.bEdgePan ? 1u << 5 : 0u) |
        (Profile.bReducedDynamicRange ? 1u << 6 : 0u);
}

[[nodiscard]] bool EncodeProfile(
    const FEchoesPlayerProfile& Profile,
    TArray<uint8>& OutBytes,
    FString& OutError)
{
    OutBytes.Reset();
    if (!ValidateProfile(Profile, OutError))
    {
        return false;
    }
    OutBytes.Reserve(
        ProfileHeaderSize + ProfilePayloadSize + ProfileChecksumSize);
    OutBytes.Append(ProfileMagic, UE_ARRAY_COUNT(ProfileMagic));
    ProfileAppendU16(OutBytes, FEchoesPlayerProfile::SchemaVersion);
    ProfileAppendU16(OutBytes, ProfilePayloadSize);
    ProfileAppendU8(OutBytes, Profile.ActiveJourneySlot);
    ProfileAppendU8(OutBytes, PackProfileFlags(Profile));
    ProfileAppendU16(OutBytes, Profile.TutorialVerifiedMask);
    ProfileAppendI32(OutBytes, Profile.Resolution.X);
    ProfileAppendI32(OutBytes, Profile.Resolution.Y);
    ProfileAppendU8(OutBytes, static_cast<uint8>(Profile.WindowMode));
    ProfileAppendFloat(OutBytes, Profile.HudScale);
    ProfileAppendFloat(OutBytes, Profile.CameraPanSpeedScale);
    ProfileAppendFloat(OutBytes, Profile.CameraZoomScale);
    ProfileAppendFloat(OutBytes, Profile.EffectsVolume);
    ProfileAppendFloat(OutBytes, Profile.MasterVolume);
    ProfileAppendFloat(OutBytes, Profile.MusicVolume);
    ProfileAppendFloat(OutBytes, Profile.DialogueVolume);
    ProfileAppendFloat(OutBytes, Profile.InterfaceVolume);
    ProfileAppendFloat(OutBytes, Profile.AmbienceVolume);
    ProfileAppendU32(
        OutBytes,
        FCrc::MemCrc32(OutBytes.GetData(), OutBytes.Num()));
    return true;
}

[[nodiscard]] bool DecodeProfile(
    const TArray<uint8>& Bytes,
    FEchoesPlayerProfile& OutProfile,
    FString& OutError)
{
    const int32 ExpectedSize =
        ProfileHeaderSize + ProfilePayloadSize + ProfileChecksumSize;
    if (Bytes.Num() < ExpectedSize)
    {
        OutError = TEXT("[PROFILE_TRUNCATED] The player profile is incomplete.");
        return false;
    }
    if (Bytes.Num() != ExpectedSize)
    {
        OutError = TEXT("[PROFILE_LENGTH_INVALID] The player profile length is not valid for its schema.");
        return false;
    }
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ProfileMagic); ++Index)
    {
        if (Bytes[Index] != ProfileMagic[Index])
        {
            OutError = TEXT("[PROFILE_MAGIC_MISMATCH] The file is not an Echoes player profile.");
            return false;
        }
    }
    int32 ChecksumOffset = Bytes.Num() - ProfileChecksumSize;
    uint32 StoredChecksum = 0;
    if (!ProfileReadU32(Bytes, ChecksumOffset, StoredChecksum) ||
        StoredChecksum !=
            FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - ProfileChecksumSize))
    {
        OutError = TEXT("[PROFILE_CHECKSUM_MISMATCH] The player profile failed integrity validation.");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(ProfileMagic);
    uint16 Version = 0;
    uint16 PayloadSize = 0;
    if (!ProfileReadU16(Bytes, Offset, Version) ||
        !ProfileReadU16(Bytes, Offset, PayloadSize))
    {
        OutError = TEXT("[PROFILE_TRUNCATED] The player profile header is incomplete.");
        return false;
    }
    if (Version != FEchoesPlayerProfile::SchemaVersion)
    {
        OutError = FString::Printf(
            TEXT("[PROFILE_VERSION_UNSUPPORTED] Supported schema is %u; found %u."),
            FEchoesPlayerProfile::SchemaVersion,
            Version);
        return false;
    }
    if (PayloadSize != ProfilePayloadSize)
    {
        OutError = TEXT("[PROFILE_LENGTH_INVALID] The declared player profile payload length is invalid.");
        return false;
    }

    FEchoesPlayerProfile Candidate;
    uint8 Flags = 0;
    uint8 EncodedWindowMode = 0;
    if (!ProfileReadU8(Bytes, Offset, Candidate.ActiveJourneySlot) ||
        !ProfileReadU8(Bytes, Offset, Flags) ||
        !ProfileReadU16(Bytes, Offset, Candidate.TutorialVerifiedMask) ||
        !ProfileReadI32(Bytes, Offset, Candidate.Resolution.X) ||
        !ProfileReadI32(Bytes, Offset, Candidate.Resolution.Y) ||
        !ProfileReadU8(Bytes, Offset, EncodedWindowMode) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.HudScale) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.CameraPanSpeedScale) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.CameraZoomScale) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.EffectsVolume) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.MasterVolume) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.MusicVolume) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.DialogueVolume) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.InterfaceVolume) ||
        !ProfileReadFloat(Bytes, Offset, Candidate.AmbienceVolume))
    {
        OutError = TEXT("[PROFILE_TRUNCATED] The player profile payload is incomplete.");
        return false;
    }
    if ((Flags & ~ProfileAllowedFlags) != 0)
    {
        OutError = TEXT("[PROFILE_FLAGS_INVALID] The player profile contains unsupported flag bits.");
        return false;
    }
    Candidate.bOnboardingOffered = (Flags & (1u << 0)) != 0;
    Candidate.bTutorialOptOut = (Flags & (1u << 1)) != 0;
    Candidate.bHighContrastHud = (Flags & (1u << 2)) != 0;
    Candidate.bReducedMotion = (Flags & (1u << 3)) != 0;
    Candidate.bReducedFlashing = (Flags & (1u << 4)) != 0;
    Candidate.bEdgePan = (Flags & (1u << 5)) != 0;
    Candidate.bReducedDynamicRange = (Flags & (1u << 6)) != 0;
    Candidate.WindowMode = static_cast<EWindowMode::Type>(EncodedWindowMode);
    if (!ValidateProfile(Candidate, OutError))
    {
        return false;
    }
    OutProfile = Candidate;
    return true;
}

[[nodiscard]] bool LoadOneProfile(
    const FString& Path,
    FEchoesPlayerProfile& OutProfile,
    FString& OutError)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path))
    {
        OutError = TEXT("[PROFILE_READ_FAILED] The player profile could not be read.");
        return false;
    }
    return DecodeProfile(Bytes, OutProfile, OutError);
}
}

void FEchoesPlayerProfile::CaptureSettings(
    const UEchoesGameUserSettings& Settings)
{
    Resolution = Settings.GetScreenResolution();
    WindowMode = Settings.GetFullscreenMode();
    HudScale = Settings.GetHudScale();
    bHighContrastHud = Settings.IsHighContrastHudEnabled();
    bReducedMotion = Settings.IsReducedMotionEnabled();
    bReducedFlashing = Settings.IsReducedFlashingEnabled();
    bEdgePan = Settings.IsEdgePanEnabled();
    CameraPanSpeedScale = Settings.GetCameraPanSpeedScale();
    CameraZoomScale = Settings.GetCameraZoomScale();
    EffectsVolume = Settings.GetEffectsVolume();
    bReducedDynamicRange = Settings.IsReducedDynamicRangeEnabled();
    MasterVolume = Settings.GetMasterVolume();
    MusicVolume = Settings.GetMusicVolume();
    DialogueVolume = Settings.GetDialogueVolume();
    InterfaceVolume = Settings.GetInterfaceVolume();
    AmbienceVolume = Settings.GetAmbienceVolume();
}

bool FEchoesPlayerProfile::ApplySettings(
    UEchoesGameUserSettings& Settings,
    FString& OutError) const
{
    if (!ValidateProfile(*this, OutError))
    {
        return false;
    }
    Settings.SetScreenResolution(Resolution);
    Settings.SetFullscreenMode(WindowMode);
    Settings.SetHudScale(HudScale);
    Settings.SetHighContrastHudEnabled(bHighContrastHud);
    Settings.SetReducedMotionEnabled(bReducedMotion);
    Settings.SetReducedFlashingEnabled(bReducedFlashing);
    Settings.SetEdgePanEnabled(bEdgePan);
    Settings.SetCameraPanSpeedScale(CameraPanSpeedScale);
    Settings.SetCameraZoomScale(CameraZoomScale);
    Settings.SetEffectsVolume(EffectsVolume);
    Settings.SetReducedDynamicRangeEnabled(bReducedDynamicRange);
    Settings.SetMasterVolume(MasterVolume);
    Settings.SetMusicVolume(MusicVolume);
    Settings.SetDialogueVolume(DialogueVolume);
    Settings.SetInterfaceVolume(InterfaceVolume);
    Settings.SetAmbienceVolume(AmbienceVolume);
    Settings.ValidateSettings();
    OutError.Reset();
    return true;
}

bool FEchoesPlayerProfile::IsTutorialMasteryComplete() const
{
    if (!IsOrderedTutorialMask(TutorialVerifiedMask))
    {
        return false;
    }
    TArray<FEchoesTutorialLessonFacts> Facts;
    Facts.SetNum(EchoesTutorialLessonCount);
    for (int32 Index = 0; Index < EchoesTutorialLessonCount; ++Index)
    {
        const bool bVerified =
            (TutorialVerifiedMask & (1u << Index)) != 0;
        Facts[Index].LessonOrdinal = Index;
        Facts[Index].bCurriculumActive = true;
        Facts[Index].bLessonOpened = bVerified;
        Facts[Index].bActionObserved = bVerified;
        Facts[Index].bAuthoritativeStateVerified = bVerified;
    }
    return FEchoesTutorialCurriculumModel::DetermineCurriculumState(Facts)
        .bMasteryComplete;
}

FString FEchoesPlayerProfileStore::GetDefaultPath()
{
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("Profile.sav"));
}

bool FEchoesPlayerProfileStore::LoadWithBackup(
    const FString& Path,
    FEchoesPlayerProfile& OutProfile,
    bool& OutExists,
    FString& OutFeedback)
{
    const FString BackupPath = Path + TEXT(".bak");
    const bool bPrimaryExists = IFileManager::Get().FileExists(*Path);
    const bool bBackupExists = IFileManager::Get().FileExists(*BackupPath);
    OutExists = bPrimaryExists || bBackupExists;
    if (!OutExists)
    {
        OutFeedback = TEXT("[PROFILE_NOT_FOUND] No persistent player profile exists.");
        return true;
    }

    FEchoesPlayerProfile Candidate;
    FString PrimaryFailure;
    if (bPrimaryExists && LoadOneProfile(Path, Candidate, PrimaryFailure))
    {
        OutProfile = Candidate;
        OutFeedback = TEXT("PLAYER PROFILE: primary record loaded.");
        return true;
    }
    FString BackupFailure;
    if (bBackupExists && LoadOneProfile(BackupPath, Candidate, BackupFailure))
    {
        OutProfile = Candidate;
        OutFeedback = TEXT("[SAVE CORRUPT: RESTORING BACKUP] PLAYER PROFILE: prior-generation backup recovered.");
        return true;
    }
    OutFeedback = FString::Printf(
        TEXT("[PROFILE_NO_VALID_GENERATION] primary=%s; backup=%s"),
        bPrimaryExists ? *PrimaryFailure : TEXT("file unavailable"),
        bBackupExists ? *BackupFailure : TEXT("file unavailable"));
    return false;
}

bool FEchoesPlayerProfileStore::SaveAtomic(
    const FString& Path,
    const FEchoesPlayerProfile& Profile,
    FString& OutFeedback)
{
    TArray<uint8> Bytes;
    if (!EncodeProfile(Profile, Bytes, OutFeedback))
    {
        return false;
    }

    IFileManager& Files = IFileManager::Get();
    const FString Directory = FPaths::GetPath(Path);
    const FString TemporaryPath = Path + TEXT(".tmp");
    const FString BackupPath = Path + TEXT(".bak");
    if (!Files.MakeDirectory(*Directory, true))
    {
        OutFeedback = TEXT("[PROFILE_DIRECTORY_FAILED] The player-profile directory could not be created.");
        return false;
    }
    Files.Delete(*TemporaryPath, false, true, true);
    if (!FFileHelper::SaveArrayToFile(Bytes, *TemporaryPath))
    {
        OutFeedback = TEXT("[PROFILE_WRITE_FAILED] The temporary player profile could not be written.");
        return false;
    }

    FEchoesPlayerProfile Verification;
    FString VerificationError;
    if (!LoadOneProfile(TemporaryPath, Verification, VerificationError) ||
        !(Verification == Profile))
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = FString::Printf(
            TEXT("[PROFILE_VALIDATION_FAILED] %s"),
            VerificationError.IsEmpty()
                ? TEXT("The temporary profile did not reproduce the source state.")
                : *VerificationError);
        return false;
    }

    const bool bHadBackup = Files.FileExists(*BackupPath);
    TArray<uint8> PriorBackupBytes;
    if (bHadBackup && !FFileHelper::LoadFileToArray(PriorBackupBytes, *BackupPath))
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = TEXT("[PROFILE_BACKUP_READ_FAILED] The existing recovery generation could not be retained exactly.");
        return false;
    }

    bool bRetainedValidPrimary = false;
    if (Files.FileExists(*Path))
    {
        FEchoesPlayerProfile PriorPrimary;
        FString PriorPrimaryError;
        if (LoadOneProfile(Path, PriorPrimary, PriorPrimaryError))
        {
            if (!AtomicReplaceProfileFile(BackupPath, Path))
            {
                Files.Delete(*TemporaryPath, false, true, true);
                OutFeedback = TEXT("[PROFILE_BACKUP_FAILED] The valid prior player profile could not be retained.");
                return false;
            }
            bRetainedValidPrimary = true;
        }
    }

    bool bForceCommitFailure = false;
#if WITH_DEV_AUTOMATION_TESTS
    bForceCommitFailure = bFailNextProfileCommitForTesting;
    bFailNextProfileCommitForTesting = false;
#endif
    if (bForceCommitFailure ||
        !AtomicReplaceProfileFile(Path, TemporaryPath))
    {
        bool bPrimaryRestored = !bRetainedValidPrimary;
        if (bRetainedValidPrimary && Files.FileExists(*BackupPath))
        {
            bPrimaryRestored = AtomicReplaceProfileFile(Path, BackupPath);
        }
        Files.Delete(*TemporaryPath, false, true, true);
        bool bBackupRestored = !bHadBackup;
        if (bPrimaryRestored && bHadBackup)
        {
            bBackupRestored =
                FFileHelper::SaveArrayToFile(PriorBackupBytes, *TemporaryPath) &&
                AtomicReplaceProfileFile(BackupPath, TemporaryPath);
        }
        OutFeedback = bPrimaryRestored && bBackupRestored
            ? TEXT("[PROFILE_COMMIT_FAILED] The validated profile was not committed; the prior primary and recovery generation remain active.")
            : TEXT("[PROFILE_ROLLBACK_FAILED] The validated profile was not committed and the prior generation set could not be restored completely.");
        return false;
    }
    OutFeedback = TEXT("PLAYER PROFILE: validated generation committed.");
    return true;
}

bool FEchoesPlayerProfileStore::ResetPreservingInvalidGenerations(
    const FString& Path,
    const FEchoesPlayerProfile& Defaults,
    FString& OutFeedback)
{
    TArray<uint8> DefaultBytes;
    if (!EncodeProfile(Defaults, DefaultBytes, OutFeedback))
    {
        OutFeedback = FString::Printf(
            TEXT("[PROFILE_RESET_DEFAULTS_INVALID] %s"),
            *OutFeedback);
        return false;
    }

    struct FInvalidGenerationArchive
    {
        FString OriginalPath;
        FString ArchivePath;
    };
    TArray<FInvalidGenerationArchive> InvalidGenerations;
    const FString GenerationPaths[] = {
        Path,
        Path + TEXT(".bak"),
        Path + TEXT(".tmp")};
    for (const FString& GenerationPath : GenerationPaths)
    {
        if (!IFileManager::Get().FileExists(*GenerationPath))
        {
            continue;
        }
        FEchoesPlayerProfile Existing;
        FString ValidationError;
        if (GenerationPath != Path + TEXT(".tmp") &&
            LoadOneProfile(GenerationPath, Existing, ValidationError))
        {
            OutFeedback = FString::Printf(
                TEXT("[PROFILE_RESET_REFUSED_VALID_GENERATION] A valid player-profile generation remains at %s; use ordinary load or recovery instead."),
                *GenerationPath);
            return false;
        }
        FInvalidGenerationArchive& Archive =
            InvalidGenerations.AddDefaulted_GetRef();
        Archive.OriginalPath = GenerationPath;
        do
        {
            Archive.ArchivePath = FString::Printf(
                TEXT("%s.archived-%s"),
                *GenerationPath,
                *FGuid::NewGuid().ToString(EGuidFormats::Digits));
        }
        while (IFileManager::Get().FileExists(*Archive.ArchivePath));
    }

    int32 ArchivedCount = 0;
    for (; ArchivedCount < InvalidGenerations.Num(); ++ArchivedCount)
    {
        const FInvalidGenerationArchive& Archive =
            InvalidGenerations[ArchivedCount];
        if (!AtomicReplaceProfileFile(
                Archive.ArchivePath,
                Archive.OriginalPath))
        {
            bool bRollbackSucceeded = true;
            for (int32 Index = ArchivedCount - 1; Index >= 0; --Index)
            {
                const FInvalidGenerationArchive& Prior =
                    InvalidGenerations[Index];
                bRollbackSucceeded =
                    AtomicReplaceProfileFile(
                        Prior.OriginalPath,
                        Prior.ArchivePath) &&
                    bRollbackSucceeded;
            }
            OutFeedback = bRollbackSucceeded
                ? TEXT("[PROFILE_RESET_ARCHIVE_FAILED] Invalid player-profile generations could not be archived; every original generation remains active.")
                : TEXT("[PROFILE_RESET_ARCHIVE_ROLLBACK_FAILED] Invalid player-profile generations could not be archived and the original generation set could not be restored completely.");
            return false;
        }
    }

    FString SaveFeedback;
    if (!SaveAtomic(Path, Defaults, SaveFeedback))
    {
        IFileManager& Files = IFileManager::Get();
        Files.Delete(*Path, false, true, true);
        Files.Delete(*(Path + TEXT(".bak")), false, true, true);
        Files.Delete(*(Path + TEXT(".tmp")), false, true, true);
        bool bRollbackSucceeded = true;
        for (int32 Index = InvalidGenerations.Num() - 1;
             Index >= 0;
             --Index)
        {
            const FInvalidGenerationArchive& Archive =
                InvalidGenerations[Index];
            bRollbackSucceeded =
                AtomicReplaceProfileFile(
                    Archive.OriginalPath,
                    Archive.ArchivePath) &&
                bRollbackSucceeded;
        }
        OutFeedback = bRollbackSucceeded
            ? FString::Printf(
                  TEXT("[PROFILE_RESET_COMMIT_FAILED] %s Original invalid generations were restored exactly."),
                  *SaveFeedback)
            : FString::Printf(
                  TEXT("[PROFILE_RESET_ROLLBACK_FAILED] %s The original invalid generation set could not be restored completely."),
                  *SaveFeedback);
        return false;
    }

    OutFeedback = FString::Printf(
        TEXT("PLAYER PROFILE: reset committed; %d prior generation%s preserved in unique archives."),
        InvalidGenerations.Num(),
        InvalidGenerations.Num() == 1 ? TEXT("") : TEXT("s"));
    return true;
}

#if WITH_DEV_AUTOMATION_TESTS
void FEchoesPlayerProfileStore::FailNextCommitForTesting()
{
    bFailNextProfileCommitForTesting = true;
}
#endif
