#include "EchoesCampaignProgress.h"

#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
constexpr uint8 CampaignMagic[] = {
    'E', 'C', 'H', 'O', 'C', 'P', 'G', '1'};
constexpr int32 HeaderSize = 12;
constexpr int32 RecordSize = 24;
constexpr int32 ChecksumSize = 4;
constexpr uint8 AllWellChoicesMask = 0x07;
constexpr uint8 PrologueCompletionFacts =
    static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
    static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
    static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
constexpr uint8 SevenAccountsCompletionFacts =
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::WaystoneRootedAtAnchor) |
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::MemoryBearerArrived) |
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesSevenAccountsCompletionFact::PriorDecisionConsumed);
constexpr uint8 CityReserveCompletionFacts =
    static_cast<uint8>(EEchoesCityReserveCompletionFact::LifeSupportPowered) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::TransitPowered) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::ArchivePowered) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesCityReserveCompletionFact::PriorLedgerConsumed);
constexpr uint8 UnburiedRoadCompletionFacts =
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::WaystoneRootedAtRoadhead) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::ListeningSpineRaised) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::MemoryShardRecovered) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::PriorLedgerConsumed);
constexpr uint8 TermsOfContinuanceCompletionFacts =
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::MeridianRelaySynchronized) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::KharuunSpineSynchronized) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::ContinuanceWindowHeld) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::BothWitnessesExtracted) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::PriorLedgerConsumed);
constexpr uint8 NamesWithoutBirthsCompletionFacts =
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::CensusEvidenceLocated) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::ArchivePowered) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::BothCiviliansSheltered) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::EvidenceExtracted) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::PriorLedgerConsumed);
constexpr uint8 ShapeOfSilenceCompletionFacts =
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::WaystoneRootedAtListeningAnchor) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::ListeningSpineRaised) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::BothMemoryWitnessesPositioned) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::OruunReachedConfluence) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::LocalCoreSurvived) |
    static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::PriorLedgerConsumed);

void AppendU8(TArray<uint8>& Bytes, uint8 Value)
{
    Bytes.Add(Value);
}

void AppendU16(TArray<uint8>& Bytes, uint16 Value)
{
    Bytes.Add(static_cast<uint8>(Value));
    Bytes.Add(static_cast<uint8>(Value >> 8));
}

void AppendU32(TArray<uint8>& Bytes, uint32 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

void AppendU64(TArray<uint8>& Bytes, uint64 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

bool ReadU8(const TArray<uint8>& Bytes, int32& Offset, uint8& OutValue)
{
    if (!Bytes.IsValidIndex(Offset))
    {
        return false;
    }
    OutValue = Bytes[Offset++];
    return true;
}

bool ReadU16(const TArray<uint8>& Bytes, int32& Offset, uint16& OutValue)
{
    if (Offset < 0 || Offset + 2 > Bytes.Num())
    {
        return false;
    }
    OutValue = static_cast<uint16>(Bytes[Offset]) |
        static_cast<uint16>(Bytes[Offset + 1]) << 8;
    Offset += 2;
    return true;
}

bool ReadU32(const TArray<uint8>& Bytes, int32& Offset, uint32& OutValue)
{
    if (Offset < 0 || Offset + 4 > Bytes.Num())
    {
        return false;
    }
    OutValue = 0;
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        OutValue |= static_cast<uint32>(Bytes[Offset + ByteIndex])
            << (ByteIndex * 8);
    }
    Offset += 4;
    return true;
}

bool ReadU64(const TArray<uint8>& Bytes, int32& Offset, uint64& OutValue)
{
    if (Offset < 0 || Offset + 8 > Bytes.Num())
    {
        return false;
    }
    OutValue = 0;
    for (int32 ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
    {
        OutValue |= static_cast<uint64>(Bytes[Offset + ByteIndex])
            << (ByteIndex * 8);
    }
    Offset += 8;
    return true;
}

uint8 ChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

bool ValidateRecord(
    const FEchoesCampaignDecisionRecord& Record,
    FString& OutError)
{
    if (Record.Mission != EEchoesCampaignMissionId::WhatTheLedgerKeeps &&
        Record.Mission != EEchoesCampaignMissionId::SevenAccountsOfRain &&
        Record.Mission != EEchoesCampaignMissionId::ACityOnReserve &&
        Record.Mission != EEchoesCampaignMissionId::TheUnburiedRoad &&
        Record.Mission != EEchoesCampaignMissionId::TermsOfContinuance &&
        Record.Mission != EEchoesCampaignMissionId::NamesWithoutBirths &&
        Record.Mission != EEchoesCampaignMissionId::TheShapeOfSilence)
    {
        OutError = TEXT("[CAMPAIGN_UNKNOWN_MISSION] The campaign record names an unsupported mission.");
        return false;
    }
    const uint8 SelectedChoice = ChoiceMask(Record.WellChoice);
    if (SelectedChoice == 0 ||
        (Record.AvailableWellChoices & SelectedChoice) == 0 ||
        (Record.AvailableWellChoices & ~AllWellChoicesMask) != 0)
    {
        OutError = TEXT("[CAMPAIGN_INVALID_WELL_DECISION] The recorded Well decision is inconsistent.");
        return false;
    }
    const uint8 RequiredFacts =
        Record.Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? PrologueCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::SevenAccountsOfRain
            ? SevenAccountsCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::ACityOnReserve
            ? CityReserveCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::TheUnburiedRoad
            ? UnburiedRoadCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::TermsOfContinuance
            ? TermsOfContinuanceCompletionFacts
        : Record.Mission == EEchoesCampaignMissionId::NamesWithoutBirths
            ? NamesWithoutBirthsCompletionFacts
            : ShapeOfSilenceCompletionFacts;
    if ((Record.VerifiedFacts & RequiredFacts) != RequiredFacts ||
        (Record.VerifiedFacts & ~RequiredFacts) != 0)
    {
        OutError = TEXT("[CAMPAIGN_UNVERIFIED_COMPLETION] The record does not prove the mission completion contract.");
        return false;
    }
    if (Record.SimulationSnapshotVersion == 0 ||
        Record.CompletionTick == 0 || Record.FinalStateChecksum == 0)
    {
        OutError = TEXT("[CAMPAIGN_INVALID_PROVENANCE] The record lacks deterministic completion provenance.");
        return false;
    }
    return true;
}

bool TryLoadOne(
    const FString& Path,
    FEchoesCampaignProgress& OutProgress,
    FString& OutFailure)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path))
    {
        OutFailure = TEXT("file unavailable");
        return false;
    }
    return FEchoesCampaignProgressStore::Decode(
        Bytes,
        OutProgress,
        OutFailure);
}
}

const FEchoesCampaignDecisionRecord* FEchoesCampaignProgress::FindDecision(
    EEchoesCampaignMissionId Mission) const
{
    return Decisions.FindByPredicate(
        [Mission](const FEchoesCampaignDecisionRecord& Record)
        {
            return Record.Mission == Mission;
        });
}

EEchoesCampaignCommitStatus FEchoesCampaignProgress::AppendDecision(
    const FEchoesCampaignDecisionRecord& Record,
    FString& OutFeedback)
{
    if (!ValidateRecord(Record, OutFeedback))
    {
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            FindDecision(Record.Mission))
    {
        if (Existing->WellChoice == Record.WellChoice)
        {
            OutFeedback = TEXT("CAMPAIGN LEDGER: this mission decision was already recorded.");
            return EEchoesCampaignCommitStatus::AlreadyRecorded;
        }
        OutFeedback = TEXT("CAMPAIGN LEDGER: replay outcome retained, but the original irreversible decision was not rewritten.");
        return EEchoesCampaignCommitStatus::ReplayConflict;
    }
    if (Decisions.Num() >= MaximumDecisionRecords)
    {
        OutFeedback = TEXT("[CAMPAIGN_RECORD_LIMIT] The campaign ledger cannot accept another decision.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    Decisions.Add(Record);
    OutFeedback = TEXT("CAMPAIGN LEDGER: mission consequence committed.");
    return EEchoesCampaignCommitStatus::Added;
}

FString FEchoesCampaignProgressStore::GetDefaultPath()
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        TEXT("EchoesCampaignProgress.bin"));
}

bool FEchoesCampaignProgressStore::Encode(
    const FEchoesCampaignProgress& Progress,
    TArray<uint8>& OutBytes,
    FString& OutError)
{
    OutBytes.Reset();
    OutError.Reset();
    if (Progress.Decisions.Num() > FEchoesCampaignProgress::MaximumDecisionRecords)
    {
        OutError = TEXT("[CAMPAIGN_RECORD_LIMIT] Too many campaign records were supplied.");
        return false;
    }

    TSet<uint8> SeenMissions;
    for (const FEchoesCampaignDecisionRecord& Record : Progress.Decisions)
    {
        if (!ValidateRecord(Record, OutError))
        {
            return false;
        }
        const uint8 MissionValue = static_cast<uint8>(Record.Mission);
        if (SeenMissions.Contains(MissionValue))
        {
            OutError = TEXT("[CAMPAIGN_DUPLICATE_MISSION] The ledger contains more than one decision for a mission.");
            return false;
        }
        SeenMissions.Add(MissionValue);
    }

    OutBytes.Reserve(
        HeaderSize + Progress.Decisions.Num() * RecordSize + ChecksumSize);
    OutBytes.Append(CampaignMagic, UE_ARRAY_COUNT(CampaignMagic));
    AppendU16(OutBytes, FEchoesCampaignProgress::SchemaVersion);
    AppendU16(OutBytes, static_cast<uint16>(Progress.Decisions.Num()));
    for (const FEchoesCampaignDecisionRecord& Record : Progress.Decisions)
    {
        AppendU8(OutBytes, static_cast<uint8>(Record.Mission));
        AppendU8(OutBytes, static_cast<uint8>(Record.WellChoice));
        AppendU8(OutBytes, Record.AvailableWellChoices);
        AppendU8(OutBytes, Record.VerifiedFacts);
        AppendU32(OutBytes, Record.SimulationSnapshotVersion);
        AppendU64(OutBytes, Record.CompletionTick);
        AppendU64(OutBytes, Record.FinalStateChecksum);
    }
    AppendU32(
        OutBytes,
        FCrc::MemCrc32(OutBytes.GetData(), OutBytes.Num()));
    return true;
}

bool FEchoesCampaignProgressStore::Decode(
    const TArray<uint8>& Bytes,
    FEchoesCampaignProgress& OutProgress,
    FString& OutError)
{
    OutError.Reset();
    if (Bytes.Num() < HeaderSize + ChecksumSize)
    {
        OutError = TEXT("[CAMPAIGN_TRUNCATED] The campaign ledger is incomplete.");
        return false;
    }
    for (int32 MagicIndex = 0;
         MagicIndex < UE_ARRAY_COUNT(CampaignMagic);
         ++MagicIndex)
    {
        if (Bytes[MagicIndex] != CampaignMagic[MagicIndex])
        {
            OutError = TEXT("[CAMPAIGN_MAGIC_MISMATCH] The file is not an Echoes campaign ledger.");
            return false;
        }
    }

    int32 ChecksumOffset = Bytes.Num() - ChecksumSize;
    uint32 StoredChecksum = 0;
    if (!ReadU32(Bytes, ChecksumOffset, StoredChecksum) ||
        StoredChecksum != FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - ChecksumSize))
    {
        OutError = TEXT("[CAMPAIGN_CHECKSUM_MISMATCH] The campaign ledger failed integrity validation.");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(CampaignMagic);
    uint16 Version = 0;
    uint16 RecordCount = 0;
    if (!ReadU16(Bytes, Offset, Version) ||
        !ReadU16(Bytes, Offset, RecordCount))
    {
        OutError = TEXT("[CAMPAIGN_TRUNCATED] The campaign header is incomplete.");
        return false;
    }
    if (Version != FEchoesCampaignProgress::SchemaVersion)
    {
        OutError = FString::Printf(
            TEXT("[CAMPAIGN_VERSION_UNSUPPORTED] Expected schema %u, found %u."),
            FEchoesCampaignProgress::SchemaVersion,
            Version);
        return false;
    }
    if (RecordCount > FEchoesCampaignProgress::MaximumDecisionRecords ||
        Bytes.Num() != HeaderSize + RecordCount * RecordSize + ChecksumSize)
    {
        OutError = TEXT("[CAMPAIGN_LENGTH_INVALID] The ledger record count is inconsistent with its length.");
        return false;
    }

    FEchoesCampaignProgress Candidate;
    Candidate.Decisions.Reserve(RecordCount);
    for (uint16 RecordIndex = 0; RecordIndex < RecordCount; ++RecordIndex)
    {
        uint8 Mission = 0;
        uint8 Choice = 0;
        FEchoesCampaignDecisionRecord Record;
        if (!ReadU8(Bytes, Offset, Mission) ||
            !ReadU8(Bytes, Offset, Choice) ||
            !ReadU8(Bytes, Offset, Record.AvailableWellChoices) ||
            !ReadU8(Bytes, Offset, Record.VerifiedFacts) ||
            !ReadU32(Bytes, Offset, Record.SimulationSnapshotVersion) ||
            !ReadU64(Bytes, Offset, Record.CompletionTick) ||
            !ReadU64(Bytes, Offset, Record.FinalStateChecksum))
        {
            OutError = TEXT("[CAMPAIGN_TRUNCATED] A campaign record is incomplete.");
            return false;
        }
        Record.Mission = static_cast<EEchoesCampaignMissionId>(Mission);
        Record.WellChoice =
            static_cast<echoes::sim::FutureWellChoice>(Choice);
        FString AppendFeedback;
        const EEchoesCampaignCommitStatus Status =
            Candidate.AppendDecision(Record, AppendFeedback);
        if (Status != EEchoesCampaignCommitStatus::Added)
        {
            OutError = AppendFeedback;
            return false;
        }
    }
    OutProgress = MoveTemp(Candidate);
    return true;
}

bool FEchoesCampaignProgressStore::SaveAtomic(
    const FString& Path,
    const FEchoesCampaignProgress& Progress,
    FString& OutFeedback)
{
    TArray<uint8> Bytes;
    if (!Encode(Progress, Bytes, OutFeedback))
    {
        return false;
    }

    IFileManager& Files = IFileManager::Get();
    const FString Directory = FPaths::GetPath(Path);
    const FString TemporaryPath = Path + TEXT(".tmp");
    const FString BackupPath = Path + TEXT(".bak");
    if (!Files.MakeDirectory(*Directory, true))
    {
        OutFeedback = TEXT("[CAMPAIGN_DIRECTORY_FAILED] The campaign save directory could not be created.");
        return false;
    }
    Files.Delete(*TemporaryPath, false, true, true);
    if (!FFileHelper::SaveArrayToFile(Bytes, *TemporaryPath))
    {
        OutFeedback = TEXT("[CAMPAIGN_WRITE_FAILED] The temporary campaign ledger could not be written.");
        return false;
    }

    FEchoesCampaignProgress Verification;
    FString VerificationError;
    if (!TryLoadOne(TemporaryPath, Verification, VerificationError) ||
        Verification.Decisions != Progress.Decisions)
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = FString::Printf(
            TEXT("[CAMPAIGN_VALIDATION_FAILED] %s"),
            VerificationError.IsEmpty()
                ? TEXT("The temporary ledger did not reproduce the source state.")
                : *VerificationError);
        return false;
    }

    const bool bHadPrimary = Files.FileExists(*Path);
    bool bRetainedValidPrimary = false;
    if (bHadPrimary)
    {
        FEchoesCampaignProgress PriorPrimary;
        FString PriorPrimaryError;
        if (TryLoadOne(Path, PriorPrimary, PriorPrimaryError))
        {
            Files.Delete(*BackupPath, false, true, true);
            if (!Files.Move(*BackupPath, *Path, true, true, true, true))
            {
                Files.Delete(*TemporaryPath, false, true, true);
                OutFeedback = TEXT("[CAMPAIGN_BACKUP_FAILED] The prior campaign ledger could not be retained.");
                return false;
            }
            bRetainedValidPrimary = true;
        }
    }
    if (!Files.Move(*Path, *TemporaryPath, true, true, true, true))
    {
        if (bRetainedValidPrimary && Files.FileExists(*BackupPath))
        {
            Files.Move(*Path, *BackupPath, true, true, true, true);
        }
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = TEXT("[CAMPAIGN_COMMIT_FAILED] The validated campaign ledger was not committed.");
        return false;
    }
    OutFeedback = FString::Printf(
        TEXT("CAMPAIGN LEDGER: %d decision%s committed."),
        Progress.Decisions.Num(),
        Progress.Decisions.Num() == 1 ? TEXT("") : TEXT("s"));
    return true;
}

bool FEchoesCampaignProgressStore::LoadWithBackup(
    const FString& Path,
    FEchoesCampaignProgress& OutProgress,
    FString& OutFeedback)
{
    IFileManager& Files = IFileManager::Get();
    const FString BackupPath = Path + TEXT(".bak");
    const bool bPrimaryExists = Files.FileExists(*Path);
    const bool bBackupExists = Files.FileExists(*BackupPath);
    if (!bPrimaryExists && !bBackupExists)
    {
        OutProgress = FEchoesCampaignProgress{};
        OutFeedback = TEXT("CAMPAIGN LEDGER: no prior campaign; a new record will begin on mission completion.");
        return true;
    }

    FEchoesCampaignProgress Candidate;
    FString PrimaryFailure;
    if (bPrimaryExists && TryLoadOne(Path, Candidate, PrimaryFailure))
    {
        OutProgress = MoveTemp(Candidate);
        OutFeedback = TEXT("CAMPAIGN LEDGER: primary record loaded.");
        return true;
    }

    FString BackupFailure;
    if (bBackupExists && TryLoadOne(BackupPath, Candidate, BackupFailure))
    {
        OutProgress = MoveTemp(Candidate);
        OutFeedback = TEXT("CAMPAIGN LEDGER: prior-generation backup recovered.");
        return true;
    }

    OutFeedback = FString::Printf(
        TEXT("[CAMPAIGN_NO_VALID_LEDGER] primary=%s; backup=%s"),
        bPrimaryExists ? *PrimaryFailure : TEXT("file unavailable"),
        bBackupExists ? *BackupFailure : TEXT("file unavailable"));
    return false;
}

bool FEchoesCampaignProgressStore::LoadGeneration(
    const FString& Path,
    FEchoesCampaignProgress& OutProgress,
    FString& OutFeedback)
{
    if (!IFileManager::Get().FileExists(*Path))
    {
        OutFeedback = TEXT("[CAMPAIGN_GENERATION_UNAVAILABLE] The requested campaign generation does not exist.");
        return false;
    }
    if (!TryLoadOne(Path, OutProgress, OutFeedback))
    {
        OutFeedback = FString::Printf(
            TEXT("[CAMPAIGN_GENERATION_INVALID] %s"),
            *OutFeedback);
        return false;
    }
    OutFeedback = FString::Printf(
        TEXT("CAMPAIGN LEDGER: validated generation loaded with %d decision%s."),
        OutProgress.Decisions.Num(),
        OutProgress.Decisions.Num() == 1 ? TEXT("") : TEXT("s"));
    return true;
}
