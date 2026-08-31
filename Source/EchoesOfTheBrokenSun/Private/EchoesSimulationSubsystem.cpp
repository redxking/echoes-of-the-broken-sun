#include "EchoesSimulationSubsystem.h"

#include "EchoesContentSubsystem.h"
#include "EchoesDestructionView.h"
#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesFactionPolicy.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesPointerCombatGuardReview.h"
#include "EchoesPresentationAudioSubsystem.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Hash/xxhash.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr int32 PrototypeMapWidthTiles = 64;
constexpr int32 PrototypeMapHeightTiles = 64;
constexpr uint32 PrototypeTicksPerSecond = 20;
constexpr uint64 PrototypeSeed = 0xE0C0'B5A1ULL;
constexpr int32 MaximumCatchUpTicksPerFrame = 8;
constexpr uint64 SustainedStressQualificationSeconds = 60U * 60U;
constexpr uint64 SustainedStressQualificationTicks =
    SustainedStressQualificationSeconds * PrototypeTicksPerSecond;
constexpr uint64 SustainedStressActivityWindowTicks =
    5U * PrototypeTicksPerSecond;
constexpr uint32 SustainedStressStartupMinimumStableFrames = 20;
constexpr double SustainedStressStartupMinimumStableSeconds = 1.0;
constexpr float SustainedStressMaximumActiveDeltaSeconds = 0.25F;
constexpr uint64 SustainedStressMaximumActiveDeltaMicroseconds = 250'000;
constexpr uint64 SustainedStressRenewalsPerHeartbeat = 4;
static_assert(
    SustainedStressRenewalsPerHeartbeat == echoes::sim::kMaximumPlayers,
    "The renewal budget is one deterministic idle combatant per team and heartbeat.");
constexpr uint64 SustainedStressMaximumOrderRenewals =
    SustainedStressQualificationSeconds *
    SustainedStressRenewalsPerHeartbeat;
constexpr uint64 SustainedStressMaximumReplacementCommands = 200'000;
constexpr uint64 SustainedStressInitialCommandCount = 396;
constexpr uint64 SustainedStressProjectedCommandCeiling =
    SustainedStressInitialCommandCount +
    SustainedStressMaximumOrderRenewals +
    SustainedStressMaximumReplacementCommands;
static_assert(
    SustainedStressProjectedCommandCeiling <
        echoes::sim::kMaximumCommandLogEntries,
    "The one-hour sustained fixture must retain deterministic command-log headroom.");
constexpr int32 PrologueSiteRadiusTiles = 3;
constexpr int32 SevenAccountsSiteRadiusTiles = 3;
constexpr int32 UnburiedRoadSiteRadiusTiles = 3;
constexpr int32 TermsOfContinuanceSiteRadiusTiles = 3;
constexpr int32 NamesWithoutBirthsSiteRadiusTiles = 3;
constexpr int32 ShapeOfSilenceSiteRadiusTiles = 3;
constexpr int32 ShapeBesideUsSiteRadiusTiles = 3;
constexpr int32 ReserveAuthoritySiteRadiusTiles = 3;
constexpr int32 ChoirAtLumeReachSiteRadiusTiles = 3;
constexpr int32 NoNeutralLedgerSiteRadiusTiles = 3;
constexpr int32 FutureThatWonSiteRadiusTiles = 3;
constexpr int32 AssemblyOfTheMissingSiteRadiusTiles = 3;
constexpr int32 SeveralVoicesOneCommandSiteRadiusTiles = 3;
constexpr int32 BrokenSunSiteRadiusTiles = 3;
constexpr int32 BrokenSunConvergenceRadiusTiles = 2;
constexpr int32 CampaignMissionCount =
    static_cast<int32>(EEchoesCampaignMissionId::TheBrokenSun);
constexpr uint64 SeveralVoicesCrisisHoldTicks = 160;
constexpr uint8 ChoirAtLumeReachQuickSaveEnvelopeVersion = 1;
constexpr uint8 ChoirAtLumeReachQuickSaveMagic[] = {
    'E', 'C', 'H', 'O', 'M', '1', '0', 'Q'};
constexpr uint8 NoNeutralLedgerQuickSaveEnvelopeVersion = 1;
constexpr uint8 NoNeutralLedgerQuickSaveMagic[] = {
    'E', 'C', 'H', 'O', 'M', '1', '1', 'Q'};
constexpr uint8 FutureThatWonQuickSaveEnvelopeVersion = 1;
constexpr uint8 FutureThatWonQuickSaveMagic[] = {
    'E', 'C', 'H', 'O', 'M', '1', '2', 'Q'};
constexpr uint8 AssemblyOfTheMissingQuickSaveEnvelopeVersion = 1;
constexpr uint8 AssemblyOfTheMissingQuickSaveMagic[] = {
    'E', 'C', 'H', 'O', 'M', '1', '3', 'Q'};
constexpr uint8 SeveralVoicesOneCommandQuickSaveEnvelopeVersion = 2;
constexpr uint8 SeveralVoicesOneCommandQuickSaveMagic[] = {
    'E', 'C', 'H', 'O', 'M', '1', '4', 'Q'};
constexpr uint8 BrokenSunQuickSaveEnvelopeVersion = 2;
constexpr uint8 BrokenSunQuickSaveMagic[] = {
    'E', 'C', 'H', 'O', 'M', '1', '5', 'Q'};
constexpr uint8 QuickSaveContainerMinimumVersion = 1;
constexpr uint8 QuickSaveContainerVersion = 2;
constexpr uint8 QuickSaveContainerMagic[] = {
    'E', 'C', 'H', 'O', 'S', 'A', 'V', 'E'};

using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::Faction;
using echoes::sim::FutureWellChoice;
using echoes::sim::ResourcePool;
using echoes::sim::Terrain;
using echoes::sim::Vec2;

[[nodiscard]] bool TryGetFixedCampaignFaction(
    EEchoesOperationMode Operation,
    Faction& OutFaction)
{
    switch (Operation)
    {
        case EEchoesOperationMode::CampaignSevenAccounts:
        case EEchoesOperationMode::CampaignUnburiedRoad:
        case EEchoesOperationMode::CampaignShapeOfSilence:
        case EEchoesOperationMode::CampaignChoirAtLumeReach:
        case EEchoesOperationMode::CampaignNoNeutralLedger:
        case EEchoesOperationMode::CampaignFutureThatWon:
        case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
            OutFaction = Faction::KharuunAssemblies;
            return true;
        case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
        case EEchoesOperationMode::CampaignTheBrokenSun:
            OutFaction = Faction::HollowChoir;
            return true;
        case EEchoesOperationMode::CampaignPrologue:
        case EEchoesOperationMode::CampaignCityReserve:
        case EEchoesOperationMode::CampaignTermsOfContinuance:
        case EEchoesOperationMode::CampaignNamesWithoutBirths:
        case EEchoesOperationMode::CampaignShapeBesideUs:
        case EEchoesOperationMode::CampaignReserveAuthority:
            OutFaction = Faction::MeridianCompact;
            return true;
        default:
            return false;
    }
}

void AppendUint32LittleEndian(TArray<uint8>& Bytes, uint32 Value)
{
    Bytes.Add(static_cast<uint8>(Value & 0xFFU));
    Bytes.Add(static_cast<uint8>((Value >> 8U) & 0xFFU));
    Bytes.Add(static_cast<uint8>((Value >> 16U) & 0xFFU));
    Bytes.Add(static_cast<uint8>((Value >> 24U) & 0xFFU));
}

void AppendUint64LittleEndian(TArray<uint8>& Bytes, uint64 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

[[nodiscard]] bool ReadUint32LittleEndian(
    const TArray<uint8>& Bytes,
    int32& Offset,
    uint32& OutValue)
{
    if (Offset < 0 || Bytes.Num() - Offset < 4)
    {
        return false;
    }
    OutValue =
        static_cast<uint32>(Bytes[Offset]) |
        (static_cast<uint32>(Bytes[Offset + 1]) << 8U) |
        (static_cast<uint32>(Bytes[Offset + 2]) << 16U) |
        (static_cast<uint32>(Bytes[Offset + 3]) << 24U);
    Offset += 4;
    return true;
}

[[nodiscard]] bool ReadUint64LittleEndian(
    const TArray<uint8>& Bytes,
    int32& Offset,
    uint64& OutValue)
{
    if (Offset < 0 || Bytes.Num() - Offset < 8)
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

enum class EQuickSaveContainerRead : uint8
{
    Legacy,
    Wrapped,
    Invalid
};

[[nodiscard]] bool UsesQuickSaveContainer(EEchoesOperationMode Operation)
{
    return Operation == EEchoesOperationMode::Skirmish ||
        Operation == EEchoesOperationMode::CampaignPrologue ||
        Operation == EEchoesOperationMode::CampaignSevenAccounts ||
        Operation == EEchoesOperationMode::CampaignCityReserve ||
        Operation == EEchoesOperationMode::CampaignUnburiedRoad ||
        Operation == EEchoesOperationMode::CampaignTermsOfContinuance ||
        Operation == EEchoesOperationMode::CampaignNamesWithoutBirths ||
        Operation == EEchoesOperationMode::CampaignShapeOfSilence ||
        Operation == EEchoesOperationMode::CampaignShapeBesideUs ||
        Operation == EEchoesOperationMode::CampaignReserveAuthority;
}

[[nodiscard]] int32 CampaignCheckpointPrerequisiteRecordCount(
    EEchoesOperationMode Operation)
{
    switch (Operation)
    {
        case EEchoesOperationMode::CampaignSevenAccounts: return 1;
        case EEchoesOperationMode::CampaignCityReserve: return 2;
        case EEchoesOperationMode::CampaignUnburiedRoad: return 3;
        case EEchoesOperationMode::CampaignTermsOfContinuance: return 4;
        case EEchoesOperationMode::CampaignNamesWithoutBirths: return 5;
        case EEchoesOperationMode::CampaignShapeOfSilence: return 6;
        case EEchoesOperationMode::CampaignShapeBesideUs: return 7;
        case EEchoesOperationMode::CampaignReserveAuthority: return 8;
        default: return 0;
    }
}

[[nodiscard]] bool RequiresCampaignBranchBoundQuickSave(
    EEchoesOperationMode Operation)
{
    return CampaignCheckpointPrerequisiteRecordCount(Operation) > 0;
}

[[nodiscard]] bool BuildQuickSaveBranchIdentity(
    EEchoesOperationMode Operation,
    const FEchoesCampaignProgress& CampaignProgress,
    uint64& OutIdentity,
    FString& OutError)
{
    OutIdentity = 0;
    OutError.Reset();
    if (!RequiresCampaignBranchBoundQuickSave(Operation))
    {
        return true;
    }

    const FEchoesCampaignJourney FullJourney =
        FEchoesCampaignJourneyModel::Resolve(CampaignProgress);
    const int32 PrerequisiteRecordCount =
        CampaignCheckpointPrerequisiteRecordCount(Operation);
    if (FullJourney.State == EEchoesCampaignJourneyState::Unavailable ||
        CampaignProgress.Decisions.Num() < PrerequisiteRecordCount)
    {
        OutError = TEXT(
            "the active campaign ledger does not contain a valid prerequisite projection for this checkpoint operation");
        return false;
    }

    FEchoesCampaignProgress PrerequisiteProjection;
    PrerequisiteProjection.Decisions.Append(
        CampaignProgress.Decisions.GetData(),
        PrerequisiteRecordCount);
    const FEchoesCampaignJourney ProjectedJourney =
        FEchoesCampaignJourneyModel::Resolve(PrerequisiteProjection);
    if (ProjectedJourney.State != EEchoesCampaignJourneyState::Ready ||
        ProjectedJourney.NextOperation != Operation)
    {
        OutError = TEXT(
            "the campaign prerequisite projection does not authorize this checkpoint operation");
        return false;
    }

    TArray<uint8> LedgerBytes;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteProjection,
            LedgerBytes,
            OutError) ||
        LedgerBytes.IsEmpty())
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("the active campaign ledger could not be encoded");
        }
        return false;
    }
    OutIdentity = FXxHash64::HashBuffer(
        LedgerBytes.GetData(),
        static_cast<uint64>(LedgerBytes.Num())).Hash;
    return true;
}

[[nodiscard]] bool BuildQuickSaveContainer(
    EEchoesOperationMode Operation,
    Faction LocalFaction,
    uint64 CampaignBranchIdentity,
    const TArray<uint8>& Payload,
    TArray<uint8>& OutContainer,
    FString& OutError)
{
    OutContainer.Reset();
    OutError.Reset();
    if (Payload.IsEmpty())
    {
        OutError = TEXT("checkpoint payload is empty");
        return false;
    }
    OutContainer.Reserve(
        UE_ARRAY_COUNT(QuickSaveContainerMagic) + 20 + Payload.Num());
    OutContainer.Append(
        QuickSaveContainerMagic,
        UE_ARRAY_COUNT(QuickSaveContainerMagic));
    OutContainer.Add(QuickSaveContainerVersion);
    OutContainer.Add(static_cast<uint8>(Operation));
    OutContainer.Add(static_cast<uint8>(LocalFaction));
    OutContainer.Add(0);
    AppendUint64LittleEndian(OutContainer, CampaignBranchIdentity);
    AppendUint32LittleEndian(
        OutContainer,
        static_cast<uint32>(Payload.Num()));
    OutContainer.Append(Payload);
    AppendUint32LittleEndian(
        OutContainer,
        FCrc::MemCrc32(OutContainer.GetData(), OutContainer.Num()));
    return true;
}

[[nodiscard]] EQuickSaveContainerRead ExtractQuickSaveContainer(
    EEchoesOperationMode ExpectedOperation,
    Faction ExpectedFaction,
    uint64 ExpectedCampaignBranchIdentity,
    const TArray<uint8>& Bytes,
    TArray<uint8>& OutPayload,
    FString& OutError)
{
    OutPayload.Reset();
    OutError.Reset();
    if (Bytes.Num() < UE_ARRAY_COUNT(QuickSaveContainerMagic) ||
        FMemory::Memcmp(
            Bytes.GetData(),
            QuickSaveContainerMagic,
            UE_ARRAY_COUNT(QuickSaveContainerMagic)) != 0)
    {
        if (RequiresCampaignBranchBoundQuickSave(ExpectedOperation))
        {
            OutError = TEXT(
                "[LOAD_LEDGER_BRANCH_UNBOUND] This legacy checkpoint has no campaign branch identity and cannot be loaded into the active campaign.");
            return EQuickSaveContainerRead::Invalid;
        }
        return EQuickSaveContainerRead::Legacy;
    }
    constexpr int32 VersionOneHeaderSize =
        UE_ARRAY_COUNT(QuickSaveContainerMagic) + 4 + 4;
    constexpr int32 VersionTwoHeaderSize =
        UE_ARRAY_COUNT(QuickSaveContainerMagic) + 4 + 8 + 4;
    constexpr int32 ChecksumSize = 4;
    if (Bytes.Num() < VersionOneHeaderSize + ChecksumSize)
    {
        OutError = TEXT("checkpoint container is truncated");
        return EQuickSaveContainerRead::Invalid;
    }
    int32 Offset = UE_ARRAY_COUNT(QuickSaveContainerMagic);
    const uint8 Version = Bytes[Offset++];
    const uint8 Operation = Bytes[Offset++];
    const uint8 FactionValue = Bytes[Offset++];
    const uint8 Reserved = Bytes[Offset++];
    if (Version < QuickSaveContainerMinimumVersion ||
        Version > QuickSaveContainerVersion)
    {
        OutError = TEXT("checkpoint container version is unsupported");
        return EQuickSaveContainerRead::Invalid;
    }
    uint64 CampaignBranchIdentity = 0;
    if (Version >= 2 &&
        !ReadUint64LittleEndian(Bytes, Offset, CampaignBranchIdentity))
    {
        OutError = TEXT("checkpoint container is truncated");
        return EQuickSaveContainerRead::Invalid;
    }
    uint32 PayloadLength = 0;
    const int32 HeaderSize = Version >= 2
        ? VersionTwoHeaderSize
        : VersionOneHeaderSize;
    if (!ReadUint32LittleEndian(Bytes, Offset, PayloadLength) ||
        Reserved != 0 ||
        Operation != static_cast<uint8>(ExpectedOperation) ||
        FactionValue != static_cast<uint8>(ExpectedFaction) ||
        PayloadLength > static_cast<uint32>(MAX_int32) ||
        Bytes.Num() != HeaderSize + static_cast<int32>(PayloadLength) +
                ChecksumSize)
    {
        OutError = TEXT(
            "checkpoint container does not match the active operation and faction");
        return EQuickSaveContainerRead::Invalid;
    }
    if (RequiresCampaignBranchBoundQuickSave(ExpectedOperation))
    {
        if (Version < 2)
        {
            OutError = TEXT(
                "[LOAD_LEDGER_BRANCH_UNBOUND] This checkpoint predates campaign branch binding and cannot be loaded into the active campaign.");
            return EQuickSaveContainerRead::Invalid;
        }
        if (CampaignBranchIdentity != ExpectedCampaignBranchIdentity)
        {
            OutError = TEXT(
                "[LOAD_LEDGER_BRANCH_MISMATCH] This checkpoint belongs to a different campaign ledger branch.");
            return EQuickSaveContainerRead::Invalid;
        }
    }
    else if (Version >= 2 && CampaignBranchIdentity != 0)
    {
        OutError = TEXT(
            "checkpoint container carries an unexpected campaign branch identity");
        return EQuickSaveContainerRead::Invalid;
    }
    const int32 ChecksumOffset = Bytes.Num() - ChecksumSize;
    int32 ChecksumReadOffset = ChecksumOffset;
    uint32 StoredChecksum = 0;
    if (!ReadUint32LittleEndian(
            Bytes,
            ChecksumReadOffset,
            StoredChecksum) ||
        StoredChecksum != FCrc::MemCrc32(Bytes.GetData(), ChecksumOffset))
    {
        OutError = TEXT("checkpoint container checksum is invalid");
        return EQuickSaveContainerRead::Invalid;
    }
    OutPayload.Append(
        Bytes.GetData() + HeaderSize,
        static_cast<int32>(PayloadLength));
    return EQuickSaveContainerRead::Wrapped;
}

[[nodiscard]] bool AtomicReplaceFile(
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

[[nodiscard]] bool UsesCurrentSnapshotSchema(
    const TArray<uint8>& SnapshotBytes)
{
    if (SnapshotBytes.Num() < 8 || SnapshotBytes[0] != 'E' ||
        SnapshotBytes[1] != 'B' || SnapshotBytes[2] != 'S' ||
        SnapshotBytes[3] != 'N')
    {
        return false;
    }
    int32 Offset = 4;
    uint32 Version = 0;
    return ReadUint32LittleEndian(SnapshotBytes, Offset, Version) &&
        Version == echoes::sim::kSnapshotVersion;
}

[[nodiscard]] bool UsesSupportedContinuitySnapshotSchema(
    const TArray<uint8>& SnapshotBytes)
{
    if (SnapshotBytes.Num() < 8 || SnapshotBytes[0] != 'E' ||
        SnapshotBytes[1] != 'B' || SnapshotBytes[2] != 'S' ||
        SnapshotBytes[3] != 'N')
    {
        return false;
    }
    int32 Offset = 4;
    uint32 Version = 0;
    return ReadUint32LittleEndian(SnapshotBytes, Offset, Version) &&
        Version >= 21U && Version <= echoes::sim::kSnapshotVersion;
}

[[nodiscard]] bool UsesSupportedChoirSnapshotSchema(
    const TArray<uint8>& SnapshotBytes)
{
    if (SnapshotBytes.Num() < 8 || SnapshotBytes[0] != 'E' ||
        SnapshotBytes[1] != 'B' || SnapshotBytes[2] != 'S' ||
        SnapshotBytes[3] != 'N')
    {
        return false;
    }
    int32 Offset = 4;
    uint32 Version = 0;
    return ReadUint32LittleEndian(SnapshotBytes, Offset, Version) &&
        Version >= 22U && Version <= echoes::sim::kSnapshotVersion;
}

[[nodiscard]] bool BuildChoirAtLumeReachQuickSaveEnvelope(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& SnapshotBytes,
    TArray<uint8>& OutEnvelope,
    FString& OutError)
{
    OutEnvelope.Reset();
    OutError.Reset();
    if (CampaignProgress.Decisions.Num() != 9 || SnapshotBytes.IsEmpty())
    {
        OutError = TEXT(
            "Mission 10 checkpoints require an active nine-record ledger and a non-empty snapshot.");
        return false;
    }

    TArray<uint8> LedgerBytes;
    if (!FEchoesCampaignProgressStore::Encode(
            CampaignProgress, LedgerBytes, OutError) ||
        LedgerBytes.IsEmpty())
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("The active campaign ledger could not be encoded.");
        }
        return false;
    }

    const uint64 EnvelopeSize =
        static_cast<uint64>(UE_ARRAY_COUNT(ChoirAtLumeReachQuickSaveMagic)) +
        2ULL + 8ULL + static_cast<uint64>(LedgerBytes.Num()) +
        static_cast<uint64>(SnapshotBytes.Num());
    if (EnvelopeSize > static_cast<uint64>(MAX_int32))
    {
        OutError = TEXT("The Mission 10 checkpoint envelope is too large.");
        return false;
    }

    OutEnvelope.Reserve(static_cast<int32>(EnvelopeSize));
    OutEnvelope.Append(
        ChoirAtLumeReachQuickSaveMagic,
        UE_ARRAY_COUNT(ChoirAtLumeReachQuickSaveMagic));
    OutEnvelope.Add(ChoirAtLumeReachQuickSaveEnvelopeVersion);
    OutEnvelope.Add(static_cast<uint8>(
        EEchoesOperationMode::CampaignChoirAtLumeReach));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(LedgerBytes.Num()));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(SnapshotBytes.Num()));
    OutEnvelope.Append(LedgerBytes);
    OutEnvelope.Append(SnapshotBytes);
    return true;
}

[[nodiscard]] bool ExtractChoirAtLumeReachQuickSaveSnapshot(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& Envelope,
    TArray<uint8>& OutSnapshotBytes,
    FString& OutError)
{
    OutSnapshotBytes.Reset();
    OutError.Reset();
    constexpr int32 FixedHeaderSize =
        UE_ARRAY_COUNT(ChoirAtLumeReachQuickSaveMagic) + 2 + 8;
    if (Envelope.Num() < FixedHeaderSize ||
        FMemory::Memcmp(
            Envelope.GetData(),
            ChoirAtLumeReachQuickSaveMagic,
            UE_ARRAY_COUNT(ChoirAtLumeReachQuickSaveMagic)) != 0)
    {
        OutError = TEXT(
            "checkpoint is missing the Mission 10 operation-and-ledger envelope");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(ChoirAtLumeReachQuickSaveMagic);
    if (Envelope[Offset++] != ChoirAtLumeReachQuickSaveEnvelopeVersion)
    {
        OutError = TEXT(
            "checkpoint uses an unsupported Mission 10 envelope version");
        return false;
    }
    if (Envelope[Offset++] != static_cast<uint8>(
            EEchoesOperationMode::CampaignChoirAtLumeReach))
    {
        OutError = TEXT(
            "checkpoint operation binding is not The Choir at Lume Reach");
        return false;
    }

    uint32 LedgerLength = 0;
    uint32 SnapshotLength = 0;
    if (!ReadUint32LittleEndian(Envelope, Offset, LedgerLength) ||
        !ReadUint32LittleEndian(Envelope, Offset, SnapshotLength) ||
        LedgerLength == 0 || SnapshotLength == 0 ||
        static_cast<uint64>(Offset) + static_cast<uint64>(LedgerLength) +
                static_cast<uint64>(SnapshotLength) !=
            static_cast<uint64>(Envelope.Num()))
    {
        OutError = TEXT("checkpoint Mission 10 envelope lengths are invalid");
        return false;
    }

    TArray<uint8> ActiveLedgerBytes;
    FString LedgerError;
    if (CampaignProgress.Decisions.Num() != 9 ||
        !FEchoesCampaignProgressStore::Encode(
            CampaignProgress, ActiveLedgerBytes, LedgerError) ||
        ActiveLedgerBytes.Num() != static_cast<int32>(LedgerLength) ||
        FMemory::Memcmp(
            Envelope.GetData() + Offset,
            ActiveLedgerBytes.GetData(),
            LedgerLength) != 0)
    {
        OutError = TEXT(
            "checkpoint ledger binding does not match the active nine-record campaign");
        return false;
    }

    Offset += static_cast<int32>(LedgerLength);
    OutSnapshotBytes.Append(
        Envelope.GetData() + Offset,
        static_cast<int32>(SnapshotLength));
    return true;
}

[[nodiscard]] bool BuildNoNeutralLedgerQuickSaveEnvelope(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& SnapshotBytes,
    TArray<uint8>& OutEnvelope,
    FString& OutError)
{
    OutEnvelope.Reset();
    OutError.Reset();
    if ((CampaignProgress.Decisions.Num() != 10 &&
         CampaignProgress.Decisions.Num() != 11) ||
        SnapshotBytes.IsEmpty())
    {
        OutError = TEXT(
            "Mission 11 checkpoints require an active ten-record ledger and a non-empty snapshot.");
        return false;
    }

    FEchoesCampaignProgress PrerequisiteLedger;
    PrerequisiteLedger.Decisions.Reserve(10);
    for (int32 Index = 0; Index < 10; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "Mission 11 checkpoints require the canonical M01-M10 prerequisite projection.");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(
            CampaignProgress.Decisions[Index]);
    }
    TArray<uint8> LedgerBytes;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, LedgerBytes, OutError) ||
        LedgerBytes.IsEmpty())
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("The active campaign ledger could not be encoded.");
        }
        return false;
    }

    const uint64 EnvelopeSize =
        static_cast<uint64>(UE_ARRAY_COUNT(NoNeutralLedgerQuickSaveMagic)) +
        2ULL + 8ULL + static_cast<uint64>(LedgerBytes.Num()) +
        static_cast<uint64>(SnapshotBytes.Num());
    if (EnvelopeSize > static_cast<uint64>(MAX_int32))
    {
        OutError = TEXT("The Mission 11 checkpoint envelope is too large.");
        return false;
    }

    OutEnvelope.Reserve(static_cast<int32>(EnvelopeSize));
    OutEnvelope.Append(
        NoNeutralLedgerQuickSaveMagic,
        UE_ARRAY_COUNT(NoNeutralLedgerQuickSaveMagic));
    OutEnvelope.Add(NoNeutralLedgerQuickSaveEnvelopeVersion);
    OutEnvelope.Add(static_cast<uint8>(
        EEchoesOperationMode::CampaignNoNeutralLedger));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(LedgerBytes.Num()));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(SnapshotBytes.Num()));
    OutEnvelope.Append(LedgerBytes);
    OutEnvelope.Append(SnapshotBytes);
    return true;
}

[[nodiscard]] bool ExtractNoNeutralLedgerQuickSaveSnapshot(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& Envelope,
    TArray<uint8>& OutSnapshotBytes,
    FString& OutError)
{
    OutSnapshotBytes.Reset();
    OutError.Reset();
    constexpr int32 FixedHeaderSize =
        UE_ARRAY_COUNT(NoNeutralLedgerQuickSaveMagic) + 2 + 8;
    if (Envelope.Num() < FixedHeaderSize ||
        FMemory::Memcmp(
            Envelope.GetData(),
            NoNeutralLedgerQuickSaveMagic,
            UE_ARRAY_COUNT(NoNeutralLedgerQuickSaveMagic)) != 0)
    {
        OutError = TEXT(
            "checkpoint is missing the Mission 11 operation-and-ledger envelope");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(NoNeutralLedgerQuickSaveMagic);
    if (Envelope[Offset++] != NoNeutralLedgerQuickSaveEnvelopeVersion)
    {
        OutError = TEXT(
            "checkpoint uses an unsupported Mission 11 envelope version");
        return false;
    }
    if (Envelope[Offset++] != static_cast<uint8>(
            EEchoesOperationMode::CampaignNoNeutralLedger))
    {
        OutError = TEXT(
            "checkpoint operation binding is not No Neutral Ledger");
        return false;
    }

    uint32 LedgerLength = 0;
    uint32 SnapshotLength = 0;
    if (!ReadUint32LittleEndian(Envelope, Offset, LedgerLength) ||
        !ReadUint32LittleEndian(Envelope, Offset, SnapshotLength) ||
        LedgerLength == 0 || SnapshotLength == 0 ||
        static_cast<uint64>(Offset) + static_cast<uint64>(LedgerLength) +
                static_cast<uint64>(SnapshotLength) !=
            static_cast<uint64>(Envelope.Num()))
    {
        OutError = TEXT("checkpoint Mission 11 envelope lengths are invalid");
        return false;
    }

    FEchoesCampaignProgress PrerequisiteLedger;
    if ((CampaignProgress.Decisions.Num() != 10 &&
         CampaignProgress.Decisions.Num() != 11))
    {
        OutError = TEXT(
            "checkpoint ledger binding requires ten prerequisites and an optional Mission 11 receipt");
        return false;
    }
    PrerequisiteLedger.Decisions.Reserve(10);
    for (int32 Index = 0; Index < 10; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "checkpoint ledger binding does not contain the canonical M01-M10 projection");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(CampaignProgress.Decisions[Index]);
    }
    TArray<uint8> ActiveLedgerBytes;
    FString LedgerError;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, ActiveLedgerBytes, LedgerError) ||
        ActiveLedgerBytes.Num() != static_cast<int32>(LedgerLength) ||
        FMemory::Memcmp(
            Envelope.GetData() + Offset,
            ActiveLedgerBytes.GetData(),
            LedgerLength) != 0)
    {
        OutError = TEXT(
            "checkpoint ledger binding does not match the active ten-record campaign");
        return false;
    }

    Offset += static_cast<int32>(LedgerLength);
    OutSnapshotBytes.Append(
        Envelope.GetData() + Offset,
        static_cast<int32>(SnapshotLength));
    return true;
}

[[nodiscard]] bool BuildFutureThatWonQuickSaveEnvelope(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& SnapshotBytes,
    TArray<uint8>& OutEnvelope,
    FString& OutError)
{
    OutEnvelope.Reset();
    OutError.Reset();
    if ((CampaignProgress.Decisions.Num() != 11 &&
         CampaignProgress.Decisions.Num() != 12) ||
        SnapshotBytes.IsEmpty() ||
        !UsesCurrentSnapshotSchema(SnapshotBytes))
    {
        OutError = TEXT(
            "Mission 12 checkpoints require an active eleven-record ledger and a current schema-23 snapshot.");
        return false;
    }

    FEchoesCampaignProgress PrerequisiteLedger;
    PrerequisiteLedger.Decisions.Reserve(11);
    for (int32 Index = 0; Index < 11; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "Mission 12 checkpoints require the canonical M01-M11 prerequisite projection.");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(
            CampaignProgress.Decisions[Index]);
    }

    TArray<uint8> LedgerBytes;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, LedgerBytes, OutError) ||
        LedgerBytes.IsEmpty())
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("The active campaign ledger could not be encoded.");
        }
        return false;
    }

    const uint64 EnvelopeSize =
        static_cast<uint64>(UE_ARRAY_COUNT(FutureThatWonQuickSaveMagic)) +
        2ULL + 8ULL + static_cast<uint64>(LedgerBytes.Num()) +
        static_cast<uint64>(SnapshotBytes.Num());
    if (EnvelopeSize > static_cast<uint64>(MAX_int32))
    {
        OutError = TEXT("The Mission 12 checkpoint envelope is too large.");
        return false;
    }

    OutEnvelope.Reserve(static_cast<int32>(EnvelopeSize));
    OutEnvelope.Append(
        FutureThatWonQuickSaveMagic,
        UE_ARRAY_COUNT(FutureThatWonQuickSaveMagic));
    OutEnvelope.Add(FutureThatWonQuickSaveEnvelopeVersion);
    OutEnvelope.Add(static_cast<uint8>(
        EEchoesOperationMode::CampaignFutureThatWon));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(LedgerBytes.Num()));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(SnapshotBytes.Num()));
    OutEnvelope.Append(LedgerBytes);
    OutEnvelope.Append(SnapshotBytes);
    return true;
}

[[nodiscard]] bool ExtractFutureThatWonQuickSaveSnapshot(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& Envelope,
    TArray<uint8>& OutSnapshotBytes,
    FString& OutError)
{
    OutSnapshotBytes.Reset();
    OutError.Reset();
    constexpr int32 FixedHeaderSize =
        UE_ARRAY_COUNT(FutureThatWonQuickSaveMagic) + 2 + 8;
    if (Envelope.Num() < FixedHeaderSize ||
        FMemory::Memcmp(
            Envelope.GetData(),
            FutureThatWonQuickSaveMagic,
            UE_ARRAY_COUNT(FutureThatWonQuickSaveMagic)) != 0)
    {
        OutError = TEXT(
            "checkpoint is missing the Mission 12 operation-and-ledger envelope");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(FutureThatWonQuickSaveMagic);
    if (Envelope[Offset++] != FutureThatWonQuickSaveEnvelopeVersion)
    {
        OutError = TEXT(
            "checkpoint uses an unsupported Mission 12 envelope version");
        return false;
    }
    if (Envelope[Offset++] != static_cast<uint8>(
            EEchoesOperationMode::CampaignFutureThatWon))
    {
        OutError = TEXT(
            "checkpoint operation binding is not The Future That Won");
        return false;
    }

    uint32 LedgerLength = 0;
    uint32 SnapshotLength = 0;
    if (!ReadUint32LittleEndian(Envelope, Offset, LedgerLength) ||
        !ReadUint32LittleEndian(Envelope, Offset, SnapshotLength) ||
        LedgerLength == 0 || SnapshotLength == 0 ||
        static_cast<uint64>(Offset) + static_cast<uint64>(LedgerLength) +
                static_cast<uint64>(SnapshotLength) !=
            static_cast<uint64>(Envelope.Num()))
    {
        OutError = TEXT("checkpoint Mission 12 envelope lengths are invalid");
        return false;
    }

    if (CampaignProgress.Decisions.Num() != 11 &&
        CampaignProgress.Decisions.Num() != 12)
    {
        OutError = TEXT(
            "checkpoint ledger binding requires eleven prerequisites and an optional Mission 12 receipt");
        return false;
    }
    FEchoesCampaignProgress PrerequisiteLedger;
    PrerequisiteLedger.Decisions.Reserve(11);
    for (int32 Index = 0; Index < 11; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "checkpoint ledger binding does not contain the canonical M01-M11 projection");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(CampaignProgress.Decisions[Index]);
    }

    TArray<uint8> ActiveLedgerBytes;
    FString LedgerError;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, ActiveLedgerBytes, LedgerError) ||
        ActiveLedgerBytes.Num() != static_cast<int32>(LedgerLength) ||
        FMemory::Memcmp(
            Envelope.GetData() + Offset,
            ActiveLedgerBytes.GetData(),
            LedgerLength) != 0)
    {
        OutError = TEXT(
            "checkpoint ledger binding does not match the active eleven-record campaign");
        return false;
    }

    Offset += static_cast<int32>(LedgerLength);
    OutSnapshotBytes.Append(
        Envelope.GetData() + Offset,
        static_cast<int32>(SnapshotLength));
    if (!UsesSupportedContinuitySnapshotSchema(OutSnapshotBytes))
    {
        OutSnapshotBytes.Reset();
        OutError = TEXT(
            "Mission 12 checkpoints require supported snapshot schema 21 through 23 continuity state");
        return false;
    }
    return true;
}

[[nodiscard]] bool BuildAssemblyOfTheMissingQuickSaveEnvelope(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& SnapshotBytes,
    TArray<uint8>& OutEnvelope,
    FString& OutError)
{
    OutEnvelope.Reset();
    OutError.Reset();
    if ((CampaignProgress.Decisions.Num() != 12 &&
         CampaignProgress.Decisions.Num() != 13) ||
        SnapshotBytes.IsEmpty() ||
        !UsesCurrentSnapshotSchema(SnapshotBytes))
    {
        OutError = TEXT(
            "Mission 13 checkpoints require an active twelve-record ledger and a current schema-23 snapshot.");
        return false;
    }

    FEchoesCampaignProgress PrerequisiteLedger;
    PrerequisiteLedger.Decisions.Reserve(12);
    for (int32 Index = 0; Index < 12; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "Mission 13 checkpoints require the canonical M01-M12 prerequisite projection.");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(CampaignProgress.Decisions[Index]);
    }

    TArray<uint8> LedgerBytes;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, LedgerBytes, OutError) ||
        LedgerBytes.IsEmpty())
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("The active campaign ledger could not be encoded.");
        }
        return false;
    }

    const uint64 EnvelopeSize =
        static_cast<uint64>(
            UE_ARRAY_COUNT(AssemblyOfTheMissingQuickSaveMagic)) +
        2ULL + 8ULL + static_cast<uint64>(LedgerBytes.Num()) +
        static_cast<uint64>(SnapshotBytes.Num());
    if (EnvelopeSize > static_cast<uint64>(MAX_int32))
    {
        OutError = TEXT("The Mission 13 checkpoint envelope is too large.");
        return false;
    }

    OutEnvelope.Reserve(static_cast<int32>(EnvelopeSize));
    OutEnvelope.Append(
        AssemblyOfTheMissingQuickSaveMagic,
        UE_ARRAY_COUNT(AssemblyOfTheMissingQuickSaveMagic));
    OutEnvelope.Add(AssemblyOfTheMissingQuickSaveEnvelopeVersion);
    OutEnvelope.Add(static_cast<uint8>(
        EEchoesOperationMode::CampaignAssemblyOfTheMissing));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(LedgerBytes.Num()));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(SnapshotBytes.Num()));
    OutEnvelope.Append(LedgerBytes);
    OutEnvelope.Append(SnapshotBytes);
    return true;
}

[[nodiscard]] bool ExtractAssemblyOfTheMissingQuickSaveSnapshot(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& Envelope,
    TArray<uint8>& OutSnapshotBytes,
    FString& OutError)
{
    OutSnapshotBytes.Reset();
    OutError.Reset();
    constexpr int32 FixedHeaderSize =
        UE_ARRAY_COUNT(AssemblyOfTheMissingQuickSaveMagic) + 2 + 8;
    if (Envelope.Num() < FixedHeaderSize ||
        FMemory::Memcmp(
            Envelope.GetData(),
            AssemblyOfTheMissingQuickSaveMagic,
            UE_ARRAY_COUNT(AssemblyOfTheMissingQuickSaveMagic)) != 0)
    {
        OutError = TEXT(
            "checkpoint is missing the Mission 13 operation-and-ledger envelope");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(AssemblyOfTheMissingQuickSaveMagic);
    if (Envelope[Offset++] !=
        AssemblyOfTheMissingQuickSaveEnvelopeVersion)
    {
        OutError = TEXT(
            "checkpoint uses an unsupported Mission 13 envelope version");
        return false;
    }
    if (Envelope[Offset++] != static_cast<uint8>(
            EEchoesOperationMode::CampaignAssemblyOfTheMissing))
    {
        OutError = TEXT(
            "checkpoint operation binding is not Assembly of the Missing");
        return false;
    }

    uint32 LedgerLength = 0;
    uint32 SnapshotLength = 0;
    if (!ReadUint32LittleEndian(Envelope, Offset, LedgerLength) ||
        !ReadUint32LittleEndian(Envelope, Offset, SnapshotLength) ||
        LedgerLength == 0 || SnapshotLength == 0 ||
        static_cast<uint64>(Offset) + static_cast<uint64>(LedgerLength) +
                static_cast<uint64>(SnapshotLength) !=
            static_cast<uint64>(Envelope.Num()))
    {
        OutError = TEXT("checkpoint Mission 13 envelope lengths are invalid");
        return false;
    }

    if (CampaignProgress.Decisions.Num() != 12 &&
        CampaignProgress.Decisions.Num() != 13)
    {
        OutError = TEXT(
            "checkpoint ledger binding requires twelve prerequisites and an optional Mission 13 receipt");
        return false;
    }
    FEchoesCampaignProgress PrerequisiteLedger;
    PrerequisiteLedger.Decisions.Reserve(12);
    for (int32 Index = 0; Index < 12; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "checkpoint ledger binding does not contain the canonical M01-M12 projection");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(CampaignProgress.Decisions[Index]);
    }

    TArray<uint8> ActiveLedgerBytes;
    FString LedgerError;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, ActiveLedgerBytes, LedgerError) ||
        ActiveLedgerBytes.Num() != static_cast<int32>(LedgerLength) ||
        FMemory::Memcmp(
            Envelope.GetData() + Offset,
            ActiveLedgerBytes.GetData(),
            LedgerLength) != 0)
    {
        OutError = TEXT(
            "checkpoint ledger binding does not match the active twelve-record campaign");
        return false;
    }

    Offset += static_cast<int32>(LedgerLength);
    OutSnapshotBytes.Append(
        Envelope.GetData() + Offset,
        static_cast<int32>(SnapshotLength));
    if (!UsesSupportedContinuitySnapshotSchema(OutSnapshotBytes))
    {
        OutSnapshotBytes.Reset();
        OutError = TEXT(
            "Mission 13 checkpoints require supported snapshot schema 21 through 23 continuity state");
        return false;
    }
    return true;
}

[[nodiscard]] bool BuildSeveralVoicesOneCommandQuickSaveEnvelope(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& SnapshotBytes,
    bool bCrisisHoldStarted,
    bool bCrisisContractFailed,
    TArray<uint8>& OutEnvelope,
    FString& OutError)
{
    OutEnvelope.Reset();
    OutError.Reset();
    if ((CampaignProgress.Decisions.Num() != 13 &&
         CampaignProgress.Decisions.Num() != 14) ||
        SnapshotBytes.IsEmpty() ||
        !UsesCurrentSnapshotSchema(SnapshotBytes) ||
        (bCrisisContractFailed && !bCrisisHoldStarted))
    {
        OutError = TEXT(
            "Mission 14 checkpoints require an active thirteen-record ledger and a current schema-23 snapshot.");
        return false;
    }

    FEchoesCampaignProgress PrerequisiteLedger;
    PrerequisiteLedger.Decisions.Reserve(13);
    for (int32 Index = 0; Index < 13; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "Mission 14 checkpoints require the canonical M01-M13 prerequisite projection.");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(CampaignProgress.Decisions[Index]);
    }

    TArray<uint8> LedgerBytes;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, LedgerBytes, OutError) ||
        LedgerBytes.IsEmpty())
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("The active campaign ledger could not be encoded.");
        }
        return false;
    }

    const uint64 EnvelopeSize =
        static_cast<uint64>(
            UE_ARRAY_COUNT(SeveralVoicesOneCommandQuickSaveMagic)) +
        3ULL + 8ULL + 4ULL + static_cast<uint64>(LedgerBytes.Num()) +
        static_cast<uint64>(SnapshotBytes.Num());
    if (EnvelopeSize > static_cast<uint64>(MAX_int32))
    {
        OutError = TEXT("The Mission 14 checkpoint envelope is too large.");
        return false;
    }

    OutEnvelope.Reserve(static_cast<int32>(EnvelopeSize));
    OutEnvelope.Append(
        SeveralVoicesOneCommandQuickSaveMagic,
        UE_ARRAY_COUNT(SeveralVoicesOneCommandQuickSaveMagic));
    OutEnvelope.Add(SeveralVoicesOneCommandQuickSaveEnvelopeVersion);
    OutEnvelope.Add(static_cast<uint8>(
        EEchoesOperationMode::CampaignSeveralVoicesOneCommand));
    OutEnvelope.Add(
        (bCrisisHoldStarted ? 1U : 0U) |
        (bCrisisContractFailed ? 2U : 0U));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(LedgerBytes.Num()));
    AppendUint32LittleEndian(
        OutEnvelope, static_cast<uint32>(SnapshotBytes.Num()));
    OutEnvelope.Append(LedgerBytes);
    OutEnvelope.Append(SnapshotBytes);
    AppendUint32LittleEndian(
        OutEnvelope,
        FCrc::MemCrc32(OutEnvelope.GetData(), OutEnvelope.Num()));
    return true;
}

[[nodiscard]] bool ExtractSeveralVoicesOneCommandQuickSaveSnapshot(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& Envelope,
    bool& OutCrisisHoldStarted,
    bool& OutCrisisContractFailed,
    TArray<uint8>& OutSnapshotBytes,
    FString& OutError)
{
    OutCrisisHoldStarted = false;
    OutCrisisContractFailed = false;
    OutSnapshotBytes.Reset();
    OutError.Reset();
    constexpr int32 FixedHeaderSize =
        UE_ARRAY_COUNT(SeveralVoicesOneCommandQuickSaveMagic) + 3 + 8;
    constexpr int32 EnvelopeChecksumSize = 4;
    if (Envelope.Num() < FixedHeaderSize + EnvelopeChecksumSize ||
        FMemory::Memcmp(
            Envelope.GetData(),
            SeveralVoicesOneCommandQuickSaveMagic,
            UE_ARRAY_COUNT(SeveralVoicesOneCommandQuickSaveMagic)) != 0)
    {
        OutError = TEXT(
            "checkpoint is missing the Mission 14 operation-and-ledger envelope");
        return false;
    }

    int32 ChecksumOffset = Envelope.Num() - EnvelopeChecksumSize;
    uint32 StoredEnvelopeChecksum = 0;
    if (!ReadUint32LittleEndian(
            Envelope,
            ChecksumOffset,
            StoredEnvelopeChecksum) ||
        StoredEnvelopeChecksum != FCrc::MemCrc32(
            Envelope.GetData(),
            Envelope.Num() - EnvelopeChecksumSize))
    {
        OutError = TEXT("checkpoint Mission 14 envelope checksum is invalid");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(SeveralVoicesOneCommandQuickSaveMagic);
    if (Envelope[Offset++] != SeveralVoicesOneCommandQuickSaveEnvelopeVersion)
    {
        OutError = TEXT(
            "checkpoint uses an unsupported Mission 14 envelope version");
        return false;
    }
    if (Envelope[Offset++] != static_cast<uint8>(
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand))
    {
        OutError = TEXT(
            "checkpoint operation binding is not Several Voices, One Command");
        return false;
    }
    const uint8 CrisisFlags = Envelope[Offset++];
    if ((CrisisFlags & ~3U) != 0U ||
        ((CrisisFlags & 2U) != 0U && (CrisisFlags & 1U) == 0U))
    {
        OutError = TEXT(
            "checkpoint Mission 14 crisis-contract flags are invalid");
        return false;
    }

    uint32 LedgerLength = 0;
    uint32 SnapshotLength = 0;
    if (!ReadUint32LittleEndian(Envelope, Offset, LedgerLength) ||
        !ReadUint32LittleEndian(Envelope, Offset, SnapshotLength) ||
        LedgerLength == 0 || SnapshotLength == 0 ||
        static_cast<uint64>(Offset) + static_cast<uint64>(LedgerLength) +
                static_cast<uint64>(SnapshotLength) +
                static_cast<uint64>(EnvelopeChecksumSize) !=
            static_cast<uint64>(Envelope.Num()))
    {
        OutError = TEXT("checkpoint Mission 14 envelope lengths are invalid");
        return false;
    }

    if (CampaignProgress.Decisions.Num() != 13 &&
        CampaignProgress.Decisions.Num() != 14)
    {
        OutError = TEXT(
            "checkpoint ledger binding requires thirteen prerequisites and an optional Mission 14 receipt");
        return false;
    }
    FEchoesCampaignProgress PrerequisiteLedger;
    PrerequisiteLedger.Decisions.Reserve(13);
    for (int32 Index = 0; Index < 13; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "checkpoint ledger binding does not contain the canonical M01-M13 projection");
            return false;
        }
        PrerequisiteLedger.Decisions.Add(CampaignProgress.Decisions[Index]);
    }

    TArray<uint8> ActiveLedgerBytes;
    FString LedgerError;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteLedger, ActiveLedgerBytes, LedgerError) ||
        ActiveLedgerBytes.Num() != static_cast<int32>(LedgerLength) ||
        FMemory::Memcmp(
            Envelope.GetData() + Offset,
            ActiveLedgerBytes.GetData(),
            LedgerLength) != 0)
    {
        OutError = TEXT(
            "checkpoint ledger binding does not match the active thirteen-record campaign");
        return false;
    }

    Offset += static_cast<int32>(LedgerLength);
    OutSnapshotBytes.Append(
        Envelope.GetData() + Offset,
        static_cast<int32>(SnapshotLength));
    if (!UsesSupportedChoirSnapshotSchema(OutSnapshotBytes))
    {
        OutSnapshotBytes.Reset();
        OutError = TEXT(
            "Mission 14 checkpoints require supported snapshot schema 22 or 23 Hollow Choir state");
        return false;
    }
    OutCrisisHoldStarted = (CrisisFlags & 1U) != 0U;
    OutCrisisContractFailed = (CrisisFlags & 2U) != 0U;
    return true;
}

[[nodiscard]] bool BuildBrokenSunPrerequisiteProjection(
    const FEchoesCampaignProgress& CampaignProgress,
    FEchoesCampaignProgress& OutPrerequisiteLedger,
    FEchoesBrokenSunPlan& OutPlan,
    TArray<uint8>& OutLedgerBytes,
    FString& OutError)
{
    OutPrerequisiteLedger = {};
    OutPlan = {};
    OutLedgerBytes.Reset();
    if (CampaignProgress.Decisions.Num() != 14 &&
        CampaignProgress.Decisions.Num() != 15)
    {
        OutError = TEXT(
            "Mission 15 checkpoints require fourteen prerequisites and an optional final receipt");
        return false;
    }
    OutPrerequisiteLedger.Decisions.Reserve(14);
    for (int32 Index = 0; Index < 14; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            OutError = TEXT(
                "Mission 15 checkpoints require the canonical M01-M14 prerequisite projection");
            return false;
        }
        OutPrerequisiteLedger.Decisions.Add(
            CampaignProgress.Decisions[Index]);
    }
    const FEchoesCampaignDecisionRecord& Founding =
        OutPrerequisiteLedger.Decisions[0];
    const FEchoesCampaignDecisionRecord& Reserve =
        OutPrerequisiteLedger.Decisions[8];
    const FEchoesCampaignDecisionRecord& Voices =
        OutPrerequisiteLedger.Decisions[13];
    if (!FEchoesBrokenSunMissionModel::TryPlanForLedger(
            Founding.WellChoice,
            Reserve.VerifiedFacts,
            Voices.WellChoice,
            OutPlan) ||
        !FEchoesCampaignProgressStore::Encode(
            OutPrerequisiteLedger,
            OutLedgerBytes,
            OutError) ||
        OutLedgerBytes.IsEmpty())
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT(
                "The active fourteen-record final projection could not be encoded");
        }
        return false;
    }
    return true;
}

[[nodiscard]] bool BuildBrokenSunQuickSaveEnvelope(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& SnapshotBytes,
    EEchoesFinalResolution PendingResolution,
    EEchoesFinalResolution SelectedResolution,
    bool bResolutionHoldStarted,
    bool bResolutionContractFailed,
    uint64 ResolutionStartTick,
    EntityId ApproachAnchorId,
    EntityId ResolutionConduitId,
    TArray<uint8>& OutEnvelope,
    FString& OutError)
{
    OutEnvelope.Reset();
    OutError.Reset();
    FEchoesCampaignProgress PrerequisiteLedger;
    FEchoesBrokenSunPlan Plan;
    TArray<uint8> LedgerBytes;
    if (SnapshotBytes.IsEmpty() ||
        !UsesCurrentSnapshotSchema(SnapshotBytes) ||
        !BuildBrokenSunPrerequisiteProjection(
            CampaignProgress,
            PrerequisiteLedger,
            Plan,
            LedgerBytes,
            OutError))
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT(
                "Mission 15 checkpoints require a current schema-23 snapshot");
        }
        return false;
    }
    const bool bPendingValid =
        PendingResolution == EEchoesFinalResolution::None ||
        FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            Plan,
            PendingResolution);
    const bool bSelectedValid =
        SelectedResolution == EEchoesFinalResolution::None ||
        FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            Plan,
            SelectedResolution);
    if (!bPendingValid || !bSelectedValid ||
        (SelectedResolution != EEchoesFinalResolution::None &&
         PendingResolution != SelectedResolution) ||
        (bResolutionHoldStarted &&
         (SelectedResolution == EEchoesFinalResolution::None ||
          ResolutionStartTick == 0 || ApproachAnchorId == 0 ||
          ResolutionConduitId == 0)) ||
        (!bResolutionHoldStarted &&
         (ResolutionStartTick != 0 || ResolutionConduitId != 0)) ||
        (ApproachAnchorId == 0 && ResolutionConduitId != 0))
    {
        OutError = TEXT(
            "Mission 15 checkpoint intent, hold, or failure state is inconsistent");
        return false;
    }

    const uint64 EnvelopeSize =
        static_cast<uint64>(UE_ARRAY_COUNT(BrokenSunQuickSaveMagic)) +
        6ULL + 8ULL + 8ULL + 8ULL +
        static_cast<uint64>(LedgerBytes.Num()) +
        static_cast<uint64>(SnapshotBytes.Num()) + 4ULL;
    if (EnvelopeSize > static_cast<uint64>(MAX_int32))
    {
        OutError = TEXT("The Mission 15 checkpoint envelope is too large");
        return false;
    }

    OutEnvelope.Reserve(static_cast<int32>(EnvelopeSize));
    OutEnvelope.Append(
        BrokenSunQuickSaveMagic,
        UE_ARRAY_COUNT(BrokenSunQuickSaveMagic));
    OutEnvelope.Add(BrokenSunQuickSaveEnvelopeVersion);
    OutEnvelope.Add(static_cast<uint8>(
        EEchoesOperationMode::CampaignTheBrokenSun));
    OutEnvelope.Add(static_cast<uint8>(PendingResolution));
    OutEnvelope.Add(static_cast<uint8>(SelectedResolution));
    OutEnvelope.Add(Plan.AvailableFinalResolutions);
    OutEnvelope.Add(
        (bResolutionHoldStarted ? 1U : 0U) |
        (bResolutionContractFailed ? 2U : 0U));
    AppendUint64LittleEndian(OutEnvelope, ResolutionStartTick);
    AppendUint32LittleEndian(OutEnvelope, ApproachAnchorId);
    AppendUint32LittleEndian(OutEnvelope, ResolutionConduitId);
    AppendUint32LittleEndian(
        OutEnvelope,
        static_cast<uint32>(LedgerBytes.Num()));
    AppendUint32LittleEndian(
        OutEnvelope,
        static_cast<uint32>(SnapshotBytes.Num()));
    OutEnvelope.Append(LedgerBytes);
    OutEnvelope.Append(SnapshotBytes);
    AppendUint32LittleEndian(
        OutEnvelope,
        FCrc::MemCrc32(OutEnvelope.GetData(), OutEnvelope.Num()));
    return true;
}

[[nodiscard]] bool ExtractBrokenSunQuickSaveSnapshot(
    const FEchoesCampaignProgress& CampaignProgress,
    const TArray<uint8>& Envelope,
    EEchoesFinalResolution& OutPendingResolution,
    EEchoesFinalResolution& OutSelectedResolution,
    bool& OutResolutionHoldStarted,
    bool& OutResolutionContractFailed,
    uint64& OutResolutionStartTick,
    EntityId& OutApproachAnchorId,
    EntityId& OutResolutionConduitId,
    TArray<uint8>& OutSnapshotBytes,
    FString& OutError)
{
    OutPendingResolution = EEchoesFinalResolution::None;
    OutSelectedResolution = EEchoesFinalResolution::None;
    OutResolutionHoldStarted = false;
    OutResolutionContractFailed = false;
    OutResolutionStartTick = 0;
    OutApproachAnchorId = 0;
    OutResolutionConduitId = 0;
    OutSnapshotBytes.Reset();
    OutError.Reset();
    constexpr int32 FixedHeaderSize =
        UE_ARRAY_COUNT(BrokenSunQuickSaveMagic) + 6 + 8 + 8 + 8;
    constexpr int32 EnvelopeChecksumSize = 4;
    if (Envelope.Num() < FixedHeaderSize + EnvelopeChecksumSize ||
        FMemory::Memcmp(
            Envelope.GetData(),
            BrokenSunQuickSaveMagic,
            UE_ARRAY_COUNT(BrokenSunQuickSaveMagic)) != 0)
    {
        OutError = TEXT(
            "checkpoint is missing the Mission 15 operation-and-ledger envelope");
        return false;
    }
    int32 ChecksumOffset = Envelope.Num() - EnvelopeChecksumSize;
    uint32 StoredChecksum = 0;
    if (!ReadUint32LittleEndian(
            Envelope,
            ChecksumOffset,
            StoredChecksum) ||
        StoredChecksum != FCrc::MemCrc32(
            Envelope.GetData(),
            Envelope.Num() - EnvelopeChecksumSize))
    {
        OutError = TEXT("checkpoint Mission 15 envelope checksum is invalid");
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(BrokenSunQuickSaveMagic);
    if (Envelope[Offset++] != BrokenSunQuickSaveEnvelopeVersion ||
        Envelope[Offset++] != static_cast<uint8>(
            EEchoesOperationMode::CampaignTheBrokenSun))
    {
        OutError = TEXT(
            "checkpoint version or operation binding is not The Broken Sun");
        return false;
    }
    const EEchoesFinalResolution Pending =
        static_cast<EEchoesFinalResolution>(Envelope[Offset++]);
    const EEchoesFinalResolution Selected =
        static_cast<EEchoesFinalResolution>(Envelope[Offset++]);
    const uint8 AvailableResolutions = Envelope[Offset++];
    const uint8 Flags = Envelope[Offset++];
    uint64 ResolutionStartTick = 0;
    uint32 ApproachAnchorId = 0;
    uint32 ResolutionConduitId = 0;
    uint32 LedgerLength = 0;
    uint32 SnapshotLength = 0;
    if ((Flags & ~3U) != 0U ||
        !ReadUint64LittleEndian(
            Envelope,
            Offset,
            ResolutionStartTick) ||
        !ReadUint32LittleEndian(Envelope, Offset, ApproachAnchorId) ||
        !ReadUint32LittleEndian(Envelope, Offset, ResolutionConduitId) ||
        !ReadUint32LittleEndian(Envelope, Offset, LedgerLength) ||
        !ReadUint32LittleEndian(Envelope, Offset, SnapshotLength) ||
        LedgerLength == 0 || SnapshotLength == 0 ||
        static_cast<uint64>(Offset) + static_cast<uint64>(LedgerLength) +
                static_cast<uint64>(SnapshotLength) +
                static_cast<uint64>(EnvelopeChecksumSize) !=
            static_cast<uint64>(Envelope.Num()))
    {
        OutError = TEXT("checkpoint Mission 15 fields or lengths are invalid");
        return false;
    }

    FEchoesCampaignProgress PrerequisiteLedger;
    FEchoesBrokenSunPlan Plan;
    TArray<uint8> ActiveLedgerBytes;
    FString ProjectionError;
    if (!BuildBrokenSunPrerequisiteProjection(
            CampaignProgress,
            PrerequisiteLedger,
            Plan,
            ActiveLedgerBytes,
            ProjectionError) ||
        AvailableResolutions != Plan.AvailableFinalResolutions ||
        ActiveLedgerBytes.Num() != static_cast<int32>(LedgerLength) ||
        FMemory::Memcmp(
            Envelope.GetData() + Offset,
            ActiveLedgerBytes.GetData(),
            LedgerLength) != 0)
    {
        OutError = TEXT(
            "checkpoint Mission 15 ledger, plan, or earned-ending binding does not match the active campaign");
        return false;
    }
    const bool bPendingValid =
        Pending == EEchoesFinalResolution::None ||
        FEchoesBrokenSunMissionModel::IsResolutionAvailable(Plan, Pending);
    const bool bSelectedValid =
        Selected == EEchoesFinalResolution::None ||
        FEchoesBrokenSunMissionModel::IsResolutionAvailable(Plan, Selected);
    const bool bHoldStarted = (Flags & 1U) != 0U;
    const bool bContractFailed = (Flags & 2U) != 0U;
    if (!bPendingValid || !bSelectedValid ||
        (Selected != EEchoesFinalResolution::None && Pending != Selected) ||
        (bHoldStarted &&
         (Selected == EEchoesFinalResolution::None ||
          ResolutionStartTick == 0 || ApproachAnchorId == 0 ||
          ResolutionConduitId == 0)) ||
        (!bHoldStarted &&
         (ResolutionStartTick != 0 || ResolutionConduitId != 0)) ||
        (ApproachAnchorId == 0 && ResolutionConduitId != 0))
    {
        OutError = TEXT(
            "checkpoint Mission 15 intent, hold, or failure state is invalid");
        return false;
    }

    Offset += static_cast<int32>(LedgerLength);
    OutSnapshotBytes.Append(
        Envelope.GetData() + Offset,
        static_cast<int32>(SnapshotLength));
    if (!UsesSupportedChoirSnapshotSchema(OutSnapshotBytes))
    {
        OutSnapshotBytes.Reset();
        OutError = TEXT(
            "Mission 15 checkpoints require supported snapshot schema 22 or 23 state");
        return false;
    }
    OutPendingResolution = Pending;
    OutSelectedResolution = Selected;
    OutResolutionHoldStarted = bHoldStarted;
    OutResolutionContractFailed = bContractFailed;
    OutResolutionStartTick = ResolutionStartTick;
    OutApproachAnchorId = ApproachAnchorId;
    OutResolutionConduitId = ResolutionConduitId;
    return true;
}

[[nodiscard]] const TCHAR* FactionStableName(Faction Value)
{
    switch (Value)
    {
        case Faction::MeridianCompact:
            return TEXT("MeridianCompact");
        case Faction::KharuunAssemblies:
            return TEXT("KharuunAssemblies");
        case Faction::HollowChoir:
            return TEXT("HollowChoir");
    }
    return TEXT("UnknownFaction");
}

[[nodiscard]] const TCHAR* ResearchStableName(echoes::sim::ResearchType Value)
{
    switch (Value)
    {
        case echoes::sim::ResearchType::MeridianPrismaticTargeting:
            return TEXT("MeridianPrismaticTargeting");
        case echoes::sim::ResearchType::MeridianHorizonLattice:
            return TEXT("MeridianHorizonLattice");
        case echoes::sim::ResearchType::KharuunEchoCartography:
            return TEXT("KharuunEchoCartography");
        case echoes::sim::ResearchType::KharuunAncestralEdge:
            return TEXT("KharuunAncestralEdge");
        case echoes::sim::ResearchType::ChoirHeldAlternatives:
            return TEXT("ChoirHeldAlternatives");
        case echoes::sim::ResearchType::ChoirSharedResolution:
            return TEXT("ChoirSharedResolution");
        default:
            return TEXT("None");
    }
}

[[nodiscard]] bool IsGlassScarCrossing(int32 TileX)
{
    const bool bWesternCavern = TileX >= 12 && TileX <= 15;
    const bool bFutureWellSpan = TileX >= 29 && TileX <= 35;
    const bool bEasternCavern = TileX >= 48 && TileX <= 51;
    return bWesternCavern || bFutureWellSpan || bEasternCavern;
}

[[nodiscard]] int32 ConfigureGlassScar(echoes::sim::Simulation& Simulation)
{
    int32 BlockedTiles = 0;
    for (int32 TileY = 30; TileY <= 34; ++TileY)
    {
        for (int32 TileX = 8; TileX <= 55; ++TileX)
        {
            if (IsGlassScarCrossing(TileX))
            {
                continue;
            }
            if (Simulation.SetTerrainTile(TileX, TileY, Terrain::Blocked))
            {
                ++BlockedTiles;
            }
        }
    }
    return BlockedTiles;
}

[[nodiscard]] int32 ConfigureLumeReach(
    echoes::sim::Simulation& Simulation,
    FutureWellChoice PriorChoice)
{
    int32 BlockedTiles = 0;
    const auto BlockTile = [&Simulation, &BlockedTiles](int32 TileX, int32 TileY)
    {
        if (Simulation.SetTerrainTile(TileX, TileY, Terrain::Blocked))
        {
            ++BlockedTiles;
        }
    };

    for (int32 TileY = 28; TileY <= 30; ++TileY)
    {
        for (int32 TileX = 8; TileX <= 55; ++TileX)
        {
            const bool bPublicGate =
                (TileX >= 16 && TileX <= 19) ||
                (TileX >= 30 && TileX <= 34) ||
                (TileX >= 45 && TileX <= 48);
            if (!bPublicGate)
            {
                BlockTile(TileX, TileY);
            }
        }
    }
    for (int32 TileY = 36; TileY <= 44; ++TileY)
    {
        for (int32 TileX = 20; TileX <= 25; ++TileX)
        {
            BlockTile(TileX, TileY);
        }
        for (int32 TileX = 39; TileX <= 44; ++TileX)
        {
            BlockTile(TileX, TileY);
        }
    }

    if (PriorChoice == FutureWellChoice::Harvest)
    {
        for (int32 TileY = 34; TileY <= 38; ++TileY)
        {
            for (int32 TileX = 29; TileX <= 30; ++TileX)
            {
                BlockTile(TileX, TileY);
            }
        }
    }
    else if (PriorChoice == FutureWellChoice::Preserve)
    {
        for (int32 TileY = 34; TileY <= 38; ++TileY)
        {
            for (int32 TileX = 33; TileX <= 34; ++TileX)
            {
                BlockTile(TileX, TileY);
            }
        }
    }
    else if (PriorChoice == FutureWellChoice::Reshape)
    {
        for (int32 TileY = 34; TileY <= 35; ++TileY)
        {
            for (int32 TileX = 30; TileX <= 34; ++TileX)
            {
                BlockTile(TileX, TileY);
            }
        }
    }
    return BlockedTiles;
}

[[nodiscard]] int32 ApplySevenAccountsTerrain(
    echoes::sim::Simulation& Simulation,
    FutureWellChoice Branch)
{
    int32 Delta = 0;
    if (Branch == FutureWellChoice::Harvest)
    {
        for (int32 TileY = 30; TileY <= 34; ++TileY)
        {
            for (int32 TileX = 29; TileX <= 35; ++TileX)
            {
                if (Simulation.SetTerrainTile(TileX, TileY, Terrain::Blocked))
                {
                    ++Delta;
                }
            }
        }
    }
    else if (Branch == FutureWellChoice::Reshape)
    {
        constexpr int32 OpenColumns[] = {27, 28, 36, 37};
        for (int32 TileY = 30; TileY <= 34; ++TileY)
        {
            for (const int32 TileX : OpenColumns)
            {
                if (Simulation.SetTerrainTile(TileX, TileY, Terrain::Open))
                {
                    --Delta;
                }
            }
        }
    }
    return Delta;
}

[[nodiscard]] int32 ApplyUnburiedRoadTerrain(
    echoes::sim::Simulation& Simulation,
    FutureWellChoice Branch)
{
    int32 Delta = 0;
    for (int32 TileY = 30; TileY <= 34; ++TileY)
    {
        for (int32 TileX = 8; TileX <= 55; ++TileX)
        {
            const bool bWestern = TileX >= 12 && TileX <= 15;
            const bool bCentral = TileX >= 29 && TileX <= 35;
            const bool bEastern = TileX >= 48 && TileX <= 51;
            const bool bSelected =
                (Branch == FutureWellChoice::Harvest && bWestern) ||
                (Branch == FutureWellChoice::Preserve && bCentral) ||
                (Branch == FutureWellChoice::Reshape && bEastern);
            if ((bWestern || bCentral || bEastern) && !bSelected &&
                Simulation.SetTerrainTile(TileX, TileY, Terrain::Blocked))
            {
                ++Delta;
            }
        }
    }
    return Delta;
}

[[nodiscard]] uint8 WellChoiceMask(FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return 1 << 0;
        case FutureWellChoice::Preserve: return 1 << 1;
        case FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

[[nodiscard]] bool IsWithinTiles(
    const Vec2& Position,
    const Vec2& Site,
    int32 RadiusTiles)
{
    const int64 DeltaX = static_cast<int64>(Position.x.Raw()) - Site.x.Raw();
    const int64 DeltaY = static_cast<int64>(Position.y.Raw()) - Site.y.Raw();
    const int64 RadiusRaw = Vec2::FromTiles(RadiusTiles, 0).x.Raw();
    return DeltaX * DeltaX + DeltaY * DeltaY <= RadiusRaw * RadiusRaw;
}

[[nodiscard]] bool IsPublicInterface(
    const echoes::sim::Entity* Entity,
    Faction InterfaceFaction,
    const Vec2& Site,
    bool bExpectedPowered)
{
    return Entity != nullptr &&
        Entity->owner == echoes::sim::kNeutralPlayer &&
        Entity->faction == InterfaceFaction &&
        Entity->type == EntityType::UtilityStructure &&
        Entity->position == Site && Entity->hitPoints > 0 &&
        Entity->completed && Entity->aegisPowered == bExpectedPowered &&
        Entity->attackRangeRaw == 0 && Entity->attackDamage == 0 &&
        Entity->attackPeriodTicks == 0 && Entity->visionTiles == 0;
}
}

void UEchoesSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bScenarioReady = false;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = false;
    bLoggedStressCombat = false;
    bLoggedAiExpansion = false;
    bLoggedAiRetreat = false;
    bLoggedAiPlayerView = false;
    bLoggedAiAdaptation = false;
    bLoggedAiMineralCover = false;
    bLoggedAiVibrationResponse = false;
    bResearchPresentationScenario = false;
    bResearchInterruptionPresentationScenario = false;
    bKharuunSystemsPresentationScenario = false;
    bPrologueCompletionPresentationScenario = false;
    bPointerCombatGuardPresentationScenario = false;
    bLoggedResearchPresentationActive = false;
    bLoggedResearchPresentationComplete = false;
    bLoggedResearchPresentationInterrupted = false;
    bLoggedKharuunSystemsPresentation = false;
    PrologueCompletionPresentationStage = 0;
    ProloguePresentationWorkerId = 0;
    ProloguePresentationWellId = 0;
    bSimulationPaused = false;
    bMatchResultReported = false;
    LocalFaction = Faction::MeridianCompact;
    SelectedOperation = EEchoesOperationMode::Skirmish;
    ArchiveCarrierId = 0;
    MemoryBearerId = 0;
    MigrationWaystoneId = 0;
    LifeSupportDistrictId = 0;
    TransitDistrictId = 0;
    ArchiveDistrictId = 0;
    MeridianContinuanceRelayId = 0;
    KharuunContinuanceSpineId = 0;
    MeridianContinuanceWitnessId = 0;
    KharuunContinuanceWitnessId = 0;
    TalarId = 0;
    CensusArchiveId = 0;
    FirstCivilianId = 0;
    SecondCivilianId = 0;
    OruunId = 0;
    FirstMemoryWitnessId = 0;
    SecondMemoryWitnessId = 0;
    ShapeBesideUsTalarId = 0;
    FirstStateWitnessId = 0;
    SecondStateWitnessId = 0;
    ReserveAuthorityMaraId = 0;
    ChoirAtLumeReachOruunId = 0;
    ChoirAtLumeReachWaystoneId = 0;
    ChoirAtLumeReachWellId = 0;
    NoNeutralOruunId = 0;
    NoNeutralWaystoneId = 0;
    NoNeutralLedgerWitnessId = 0;
    NoNeutralFirstDistrictInterfaceId = 0;
    NoNeutralSecondDistrictInterfaceId = 0;
    NoNeutralMeridianEvidenceInterfaceId = 0;
    NoNeutralKharuunEvidenceInterfaceId = 0;
    NoNeutralWellId = 0;
    FutureWonOruunId = 0;
    FutureWonVerifierId = 0;
    FutureWonFirstDistrictInterfaceId = 0;
    FutureWonSecondDistrictInterfaceId = 0;
    FutureWonMeridianReadbackInterfaceId = 0;
    FutureWonKharuunReadbackInterfaceId = 0;
    FutureWonDemonstratorInterfaceId = 0;
    FutureWonWellId = 0;
    AssemblyOruunId = 0;
    AssemblyVerifierId = 0;
    AssemblyMeridianPublicRecordInterfaceId = 0;
    AssemblyKharuunPublicRecordInterfaceId = 0;
    AssemblyCrownfallIndexInterfaceId = 0;
    SeveralVoicesPossibleVoiceId = 0;
    SeveralVoicesManifestVoiceId = 0;
    SeveralVoicesNemeId = 0;
    SeveralVoicesResearchLoomId = 0;
    bSeveralVoicesCrisisHoldStarted = false;
    bSeveralVoicesCrisisContractFailed = false;
    BrokenSunAccordVoiceId = 0;
    BrokenSunAccordHeavyId = 0;
    BrokenSunNemeId = 0;
    BrokenSunWorkerId = 0;
    BrokenSunMaraId = 0;
    BrokenSunOruunId = 0;
    BrokenSunTalarId = 0;
    BrokenSunApproachAnchorId = 0;
    BrokenSunResolutionConduitId = 0;
    PendingBrokenSunResolution = EEchoesFinalResolution::None;
    SelectedBrokenSunResolution = EEchoesFinalResolution::None;
    bBrokenSunResolutionHoldStarted = false;
    bBrokenSunResolutionContractFailed = false;
    BrokenSunResolutionStartTick = 0;
    CampaignProgress = FEchoesCampaignProgress{};
    CampaignBackupProgress = FEchoesCampaignProgress{};
    bCampaignBackupAvailable = false;
    CampaignProgressPath = FEchoesCampaignProgressStore::GetDefaultPath();
#if !UE_BUILD_SHIPPING
    FString CampaignPathOverride;
    if (FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesCampaignProgressPath="),
            CampaignPathOverride) &&
        !CampaignPathOverride.IsEmpty())
    {
        CampaignProgressPath = FPaths::ConvertRelativePathToFull(
            CampaignPathOverride);
    }
#endif
    FString CampaignFeedback;
    bCampaignProgressAvailable =
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignProgressPath,
            CampaignProgress,
            CampaignFeedback);
    if (bCampaignProgressAvailable)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CAMPAIGN_LEDGER_LOAD] available=true records=%d detail=%s"),
            CampaignProgress.Decisions.Num(),
            *CampaignFeedback);
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CAMPAIGN_LEDGER_LOAD] available=false records=%d detail=%s"),
            CampaignProgress.Decisions.Num(),
            *CampaignFeedback);
    }
    RefreshCampaignBackupState();
    ResearchPresentationTechnology = echoes::sim::ResearchType::None;
    FogView.Reset();
    TerrainView.Reset();
}

void UEchoesSimulationSubsystem::Deinitialize()
{
    StopPrototypeScenario();
    Super::Deinitialize();
}

TStatId UEchoesSimulationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(
        UEchoesSimulationSubsystem,
        STATGROUP_Tickables);
}

bool UEchoesSimulationSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bScenarioReady && Simulation.IsValid() && World != nullptr &&
           World->IsGameWorld() && !World->bIsTearingDown;
}

bool UEchoesSimulationSubsystem::StartPrototypeScenario()
{
    return StartScenario(false);
}

bool UEchoesSimulationSubsystem::StartStressScenario()
{
    return StartScenario(true);
}

bool UEchoesSimulationSubsystem::StartSustainedStressScenario()
{
#if UE_BUILD_SHIPPING
    UE_LOG(
        LogEchoes,
        Error,
        TEXT("[ECHOES_STRESS_SUSTAINED_FAILED] code=SHIPPING_EXCLUDED tick=0 detail=The sustained fixture is compiled out of Shipping bootstrap."));
    return false;
#else
    const bool bStarted = StartScenario(true, true);
    if (!bStarted && !bSustainedStressFailed)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_STRESS_SUSTAINED_FAILED] code=START_REJECTED tick=0 detail=The sustained fixture could not initialize."));
    }
    return bStarted;
#endif
}

echoes::sim::Vec2 UEchoesSimulationSubsystem::GetArchiveRecoverySite()
{
    return Vec2::FromTiles(22, 18);
}

echoes::sim::Vec2 UEchoesSimulationSubsystem::GetEvacuationSite()
{
    return Vec2::FromTiles(6, 17);
}

FString UEchoesSimulationSubsystem::GetOperationLabel() const
{
    return FEchoesCampaignJourneyModel::OperationDisplayName(
        SelectedOperation);
}

FEchoesCampaignJourney UEchoesSimulationSubsystem::GetCampaignJourney() const
{
    if (!bCampaignProgressAvailable)
    {
        return {};
    }

    FEchoesCampaignJourney Journey =
        FEchoesCampaignJourneyModel::Resolve(CampaignProgress);
    if (Journey.State != EEchoesCampaignJourneyState::Ready)
    {
        return Journey;
    }

    const bool bAdmitted =
        Journey.NextOperation == EEchoesOperationMode::CampaignPrologue ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignSevenAccounts &&
         IsSevenAccountsUnlocked()) ||
        (Journey.NextOperation == EEchoesOperationMode::CampaignCityReserve &&
         IsCityReserveUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignUnburiedRoad &&
         IsUnburiedRoadUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignTermsOfContinuance &&
         IsTermsOfContinuanceUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignNamesWithoutBirths &&
         IsNamesWithoutBirthsUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignShapeOfSilence &&
         IsShapeOfSilenceUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignShapeBesideUs &&
         IsShapeBesideUsUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignReserveAuthority &&
         IsReserveAuthorityUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignChoirAtLumeReach &&
         IsChoirAtLumeReachUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignNoNeutralLedger &&
         IsNoNeutralLedgerUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignFutureThatWon &&
         IsFutureThatWonUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
         IsAssemblyOfTheMissingUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
         IsSeveralVoicesOneCommandUnlocked()) ||
        (Journey.NextOperation ==
             EEchoesOperationMode::CampaignTheBrokenSun &&
         IsBrokenSunUnlocked());
    if (!bAdmitted)
    {
        Journey.State = EEchoesCampaignJourneyState::Unavailable;
        Journey.NextOperation = EEchoesOperationMode::Skirmish;
    }
    return Journey;
}

bool UEchoesSimulationSubsystem::StartScenario(
    bool bUseStressScenario,
    bool bUseSustainedStressScenario)
{
    if (bUseSustainedStressScenario && !bUseStressScenario)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_STRESS_SUSTAINED_FAILED] code=INVALID_MODE tick=0 detail=Sustained mode requires the stress fixture."));
        return false;
    }
    if (bSustainedStressFailed)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_STRESS_SUSTAINED_FAILED] code=FAILURE_LATCHED tick=%llu detail=Stop or explicitly restart the failed fixture before starting it again."),
            static_cast<unsigned long long>(
                Simulation.IsValid() ? Simulation->CurrentTick() : 0));
        return false;
    }
    if (bScenarioReady && Simulation.IsValid())
    {
        if (bStressScenario != bUseStressScenario ||
            bSustainedStressScenario != bUseSustainedStressScenario)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_SIM_MODE_CONFLICT] activeStress=%s activeSustained=%s requestedStress=%s requestedSustained=%s"),
                bStressScenario ? TEXT("true") : TEXT("false"),
                bSustainedStressScenario ? TEXT("true") : TEXT("false"),
                bUseStressScenario ? TEXT("true") : TEXT("false"),
                bUseSustainedStressScenario ? TEXT("true") : TEXT("false"));
            return false;
        }
        UE_LOG(
            LogEchoes,
            Verbose,
            TEXT("[ECHOES_SIM_ALREADY_READY] Prototype simulation start ignored."));
        return true;
    }
#if WITH_DEV_AUTOMATION_TESTS
    if (bFailNextScenarioStartForTesting)
    {
        bFailNextScenarioStartForTesting = false;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_TEST_SCENARIO_START_REJECTED] oneShot=true"));
        return false;
    }
#endif

    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        !IsSevenAccountsUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SEVEN_ACCOUNTS_LOCKED] reason=WhatTheLedgerKeeps completion required"));
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve &&
        !IsCityReserveUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CITY_RESERVE_LOCKED] reason=two consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        !IsUnburiedRoadUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_UNBURIED_ROAD_LOCKED] reason=three consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        !IsTermsOfContinuanceUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_TERMS_OF_CONTINUANCE_LOCKED] reason=four consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        !IsNamesWithoutBirthsUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_LOCKED] reason=five consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        !IsShapeOfSilenceUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SHAPE_OF_SILENCE_LOCKED] reason=six consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs &&
        !IsShapeBesideUsUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SHAPE_BESIDE_US_LOCKED] reason=seven consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority &&
        !IsReserveAuthorityUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_RESERVE_AUTHORITY_LOCKED] reason=eight consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach &&
        !IsChoirAtLumeReachUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CHOIR_AT_LUME_REACH_LOCKED] reason=nine consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger &&
        !IsNoNeutralLedgerUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NO_NEUTRAL_LEDGER_LOCKED] reason=required ordered M01-M10 campaign prefix missing"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon &&
        !IsFutureThatWonUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_FUTURE_THAT_WON_LOCKED] reason=required ordered M01-M11 campaign prefix missing"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
        !IsAssemblyOfTheMissingUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_ASSEMBLY_OF_THE_MISSING_LOCKED] reason=required ordered M01-M12 campaign prefix missing"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        !IsSeveralVoicesOneCommandUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SEVERAL_VOICES_ONE_COMMAND_LOCKED] reason=required ordered M01-M13 campaign prefix missing"));
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun &&
        !IsBrokenSunUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_THE_BROKEN_SUN_LOCKED] reason=exact ordered fourteen-record campaign required"));
        return false;
    }

#if UE_BUILD_SHIPPING
    const bool bUseResearchPresentation = false;
    const bool bUseResearchInterruptionPresentation = false;
    const bool bUseKharuunSystemsPresentation = false;
    const bool bUsePrologueCompletionPresentation = false;
    const bool bUsePointerCombatGuardPresentation = false;
    const bool bUseNetworkMatchSmoke = false;
#else
    const bool bUseResearchPresentation =
        !bUseStressScenario &&
        FParse::Param(FCommandLine::Get(), TEXT("EchoesResearchPresentation"));
    const bool bUseResearchInterruptionPresentation =
        !bUseStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesResearchInterruptionPresentation"));
    const bool bUseKharuunSystemsPresentation =
        !bUseStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesKharuunSystemsPresentation"));
    const bool bUsePrologueCompletionPresentation =
        !bUseStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesPrologueCompletionPresentation"));
    const bool bUsePointerCombatGuardPresentation =
        !bUseStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesPointerCombatGuardReview"));
    const bool bUseNetworkMatchSmoke =
        !bUseStressScenario &&
        FParse::Param(FCommandLine::Get(), TEXT("EchoesNetworkMatchSmoke"));
#endif
    const int32 PresentationModeCount =
        (bUseResearchPresentation ? 1 : 0) +
        (bUseResearchInterruptionPresentation ? 1 : 0) +
        (bUseKharuunSystemsPresentation ? 1 : 0) +
        (bUsePrologueCompletionPresentation ? 1 : 0) +
        (bUsePointerCombatGuardPresentation ? 1 : 0);
    if (PresentationModeCount > 1)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PRESENTATION_MODE_FAILED] reason=conflicting presentation modes"));
        return false;
    }
    if (bUsePointerCombatGuardPresentation)
    {
        FEchoesPointerCombatGuardReview ReviewConfiguration;
        FString RequestedVariant;
        if (!FEchoesPointerCombatGuardReview::TryFromCommandLine(
                ReviewConfiguration,
                RequestedVariant))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_POINTER_COMBAT_GUARD_REVIEW_FAILED] stage=bootstrap reason=INVALID_VARIANT requested=%s controlledNonshipping=true"),
                *RequestedVariant);
            return false;
        }
    }
    const bool bUseAnyResearchPresentation =
        bUseResearchPresentation || bUseResearchInterruptionPresentation;
    const bool bUseAnyControlledPresentation =
        bUseAnyResearchPresentation || bUseKharuunSystemsPresentation ||
        bUsePrologueCompletionPresentation ||
        bUsePointerCombatGuardPresentation || bUseNetworkMatchSmoke;
    if (bUsePrologueCompletionPresentation &&
        SelectedOperation != EEchoesOperationMode::CampaignPrologue)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_FAILED] reason=campaign prologue operation required"));
        return false;
    }

    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World != nullptr
                                            ? World->GetGameInstance()
                                            : nullptr;
    const UEchoesContentSubsystem* Content =
        GameInstance != nullptr
            ? GameInstance->GetSubsystem<UEchoesContentSubsystem>()
            : nullptr;
    if (Content == nullptr || !Content->IsReady())
    {
        const FString Reason = Content != nullptr
                                   ? Content->GetFailureReason()
                                   : TEXT("CONTENT_SUBSYSTEM_UNAVAILABLE");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_CONTENT_REJECTED] reason=%s"),
            *Reason);
        return false;
    }

    echoes::sim::SimulationConfig Config;
    Config.mapWidthTiles = PrototypeMapWidthTiles;
    Config.mapHeightTiles = PrototypeMapHeightTiles;
    Config.ticksPerSecond = PrototypeTicksPerSecond;
    Config.randomSeed = PrototypeSeed;
    Config.protectedCommandCorePlayerMask =
        bUseSustainedStressScenario ? 0x0FU : 0U;
    FString RulesError;
    if (!Content->GetCatalog().BuildSimulationRules(
            Config.ticksPerSecond,
            Config.rules,
            RulesError))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_CONTENT_REJECTED] reason=%s"),
            *RulesError);
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        Config.rules.choirCoherence.upkeepIntervalTicks <=
            SeveralVoicesCrisisHoldTicks)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SEVERAL_VOICES_TIMER_CONTRACT_REJECTED] upkeepTicks=%llu requiredGreaterThan=%llu"),
            static_cast<unsigned long long>(
                Config.rules.choirCoherence.upkeepIntervalTicks),
            static_cast<unsigned long long>(
                SeveralVoicesCrisisHoldTicks));
        return false;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_SIM_RULES_READY] version=%u sha256=%s rosterArchetypes=24 catalogUnits=%d catalogBuildings=%d technologies=%d research=authored futureWell=authored bulwarkDeployment=authored relaySupply=authored waystoneMigration=authored warformAdaptation=authored mineralCover=authored vibrationDetection=authored poweredAegis=authored choirIdentity=authored choirCoherence=authored"),
        Config.rules.version,
        *Content->GetCatalog().Sha256,
        Content->GetCatalog().Units.Num(),
        Content->GetCatalog().Buildings.Num(),
        Content->GetCatalog().Technologies.Num());

    Simulation = MakeUnique<echoes::sim::Simulation>(Config);
    const FutureWellChoice SevenAccountsBranch = GetRecordedPrologueChoice();
    const bool bLumeReach =
        SelectedOperation == EEchoesOperationMode::CampaignChoirAtLumeReach ||
        SelectedOperation == EEchoesOperationMode::CampaignNoNeutralLedger ||
        SelectedOperation == EEchoesOperationMode::CampaignFutureThatWon ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand ||
        SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun;
    const int32 BaseGlassScarBlockedTiles = bLumeReach
        ? ConfigureLumeReach(*Simulation, SevenAccountsBranch)
        : ConfigureGlassScar(*Simulation);
    const int32 ExpectedBaseBlockedTiles = bLumeReach ? 223 : 165;
    if (BaseGlassScarBlockedTiles != ExpectedBaseBlockedTiles)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_OPERATION_TERRAIN_INIT_FAILED] operation=%s blocked=%d expected=%d"),
            *GetOperationLabel(),
            BaseGlassScarBlockedTiles,
            ExpectedBaseBlockedTiles);
        Simulation.Reset();
        return false;
    }
    const int32 SevenAccountsTerrainDelta =
        SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
            ? ApplySevenAccountsTerrain(*Simulation, SevenAccountsBranch)
            : 0;
    const int32 UnburiedRoadTerrainDelta =
        (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignTermsOfContinuance ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignNamesWithoutBirths ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignShapeOfSilence ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignShapeBesideUs ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignReserveAuthority)
            ? ApplyUnburiedRoadTerrain(*Simulation, SevenAccountsBranch)
            : 0;
    const int32 GlassScarBlockedTiles =
        BaseGlassScarBlockedTiles + SevenAccountsTerrainDelta +
        UnburiedRoadTerrainDelta;
    const Faction ScenarioLocalFaction =
        bUseStressScenario ? Faction::MeridianCompact
        : SelectedOperation == EEchoesOperationMode::CampaignPrologue
            ? Faction::MeridianCompact
        : SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
            ? Faction::KharuunAssemblies
        : SelectedOperation == EEchoesOperationMode::CampaignCityReserve
            ? Faction::MeridianCompact
        : SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad
            ? Faction::KharuunAssemblies
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignTermsOfContinuance
            ? Faction::MeridianCompact
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignNamesWithoutBirths
            ? Faction::MeridianCompact
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignShapeOfSilence
            ? Faction::KharuunAssemblies
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignShapeBesideUs
            ? Faction::MeridianCompact
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignReserveAuthority
            ? Faction::MeridianCompact
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignChoirAtLumeReach
            ? Faction::KharuunAssemblies
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignNoNeutralLedger
            ? Faction::KharuunAssemblies
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignFutureThatWon
            ? Faction::KharuunAssemblies
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignAssemblyOfTheMissing
            ? Faction::KharuunAssemblies
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignSeveralVoicesOneCommand
            ? Faction::HollowChoir
        : SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun
            ? Faction::HollowChoir
            : LocalFaction;
    const Faction ScenarioOpponentFaction =
        echoes::presentation::SkirmishOpponent(ScenarioLocalFaction);
    if (bUseKharuunSystemsPresentation &&
        ScenarioLocalFaction != Faction::KharuunAssemblies)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_FAILED] reason=local faction must be KharuunAssemblies"));
        Simulation.Reset();
        return false;
    }
    if (!Simulation->AddPlayer(
            LocalPlayerId,
            ScenarioLocalFaction,
            bUseAnyControlledPresentation ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignCityReserve ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignUnburiedRoad ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignTermsOfContinuance ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignNamesWithoutBirths ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignShapeOfSilence ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignShapeBesideUs ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignReserveAuthority ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignChoirAtLumeReach ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignNoNeutralLedger
                    || SelectedOperation ==
                        EEchoesOperationMode::CampaignFutureThatWon ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignAssemblyOfTheMissing
                    || SelectedOperation ==
                        EEchoesOperationMode::CampaignSeveralVoicesOneCommand
                    || SelectedOperation ==
                        EEchoesOperationMode::CampaignTheBrokenSun
                ? ResourcePool{1000, 500}
                : ResourcePool{500, 30}) ||
        !Simulation->AddPlayer(
            OpponentPlayerId,
            ScenarioOpponentFaction,
            ResourcePool{500, 30}) ||
        ((bUseResearchInterruptionPresentation ||
          bUseKharuunSystemsPresentation) &&
         !Simulation->AddPlayer(
             2,
             ScenarioOpponentFaction,
             ResourcePool{0, 0})) ||
        (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun &&
         (!Simulation->AddPlayer(
              2,
              Faction::MeridianCompact,
              ResourcePool{0, 0}) ||
          !Simulation->AddPlayer(
              3,
              Faction::KharuunAssemblies,
              ResourcePool{0, 0}))) ||
        (bUseStressScenario &&
         (!Simulation->AddPlayer(
              2,
              bUseSustainedStressScenario
                  ? Faction::HollowChoir
                  : Faction::KharuunAssemblies,
              ResourcePool{500, 30}) ||
          !Simulation->AddPlayer(
              3,
              Faction::MeridianCompact,
              ResourcePool{500, 30}))))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_PLAYER_INIT_FAILED] Could not initialize the requested scenario players."));
        Simulation.Reset();
        return false;
    }

    bool bSpawnSucceeded = true;
    ArchiveCarrierId = 0;
    MemoryBearerId = 0;
    MigrationWaystoneId = 0;
    LifeSupportDistrictId = 0;
    TransitDistrictId = 0;
    ArchiveDistrictId = 0;
    MeridianContinuanceRelayId = 0;
    KharuunContinuanceSpineId = 0;
    MeridianContinuanceWitnessId = 0;
    KharuunContinuanceWitnessId = 0;
    TalarId = 0;
    CensusArchiveId = 0;
    FirstCivilianId = 0;
    SecondCivilianId = 0;
    OruunId = 0;
    FirstMemoryWitnessId = 0;
    SecondMemoryWitnessId = 0;
    ShapeBesideUsTalarId = 0;
    FirstStateWitnessId = 0;
    SecondStateWitnessId = 0;
    ReserveAuthorityMaraId = 0;
    ChoirAtLumeReachOruunId = 0;
    ChoirAtLumeReachWaystoneId = 0;
    ChoirAtLumeReachWellId = 0;
    NoNeutralOruunId = 0;
    NoNeutralWaystoneId = 0;
    NoNeutralLedgerWitnessId = 0;
    NoNeutralFirstDistrictInterfaceId = 0;
    NoNeutralSecondDistrictInterfaceId = 0;
    NoNeutralMeridianEvidenceInterfaceId = 0;
    NoNeutralKharuunEvidenceInterfaceId = 0;
    NoNeutralWellId = 0;
    FutureWonOruunId = 0;
    FutureWonVerifierId = 0;
    FutureWonFirstDistrictInterfaceId = 0;
    FutureWonSecondDistrictInterfaceId = 0;
    FutureWonMeridianReadbackInterfaceId = 0;
    FutureWonKharuunReadbackInterfaceId = 0;
    FutureWonDemonstratorInterfaceId = 0;
    FutureWonWellId = 0;
    AssemblyOruunId = 0;
    AssemblyVerifierId = 0;
    AssemblyMeridianPublicRecordInterfaceId = 0;
    AssemblyKharuunPublicRecordInterfaceId = 0;
    AssemblyCrownfallIndexInterfaceId = 0;
    SeveralVoicesPossibleVoiceId = 0;
    SeveralVoicesManifestVoiceId = 0;
    SeveralVoicesNemeId = 0;
    SeveralVoicesResearchLoomId = 0;
    bSeveralVoicesCrisisHoldStarted = false;
    bSeveralVoicesCrisisContractFailed = false;
    BrokenSunAccordVoiceId = 0;
    BrokenSunAccordHeavyId = 0;
    BrokenSunNemeId = 0;
    BrokenSunWorkerId = 0;
    BrokenSunMaraId = 0;
    BrokenSunOruunId = 0;
    BrokenSunTalarId = 0;
    BrokenSunApproachAnchorId = 0;
    BrokenSunResolutionConduitId = 0;
    PendingBrokenSunResolution = EEchoesFinalResolution::None;
    SelectedBrokenSunResolution = EEchoesFinalResolution::None;
    bBrokenSunResolutionHoldStarted = false;
    bBrokenSunResolutionContractFailed = false;
    BrokenSunResolutionStartTick = 0;
    SustainedStressCombatEntityIds.Reset();
    SustainedStressCombatOwners.Reset();
    SustainedStressCombatFactions.Reset();
    SustainedStressCombatTypes.Reset();
    SustainedStressCombatSpawnPositions.Reset();
    SustainedStressCommandCoreIds.fill(0);
    const auto SpawnUnit = [this, &bSpawnSucceeded](
                               uint8 Owner,
                               Faction UnitFaction,
                               EntityType Type,
                               int32 TileX,
                               int32 TileY)
    {
        const EntityId Spawned = Simulation->SpawnEntity(
            Owner,
            UnitFaction,
            Type,
            Vec2::FromTiles(TileX, TileY));
        bSpawnSucceeded &= Spawned != 0;
        if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
            Owner == LocalPlayerId && Type == EntityType::ScoutUnit)
        {
            ArchiveCarrierId = Spawned;
        }
        if ((SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts ||
             SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad ||
             SelectedOperation == EEchoesOperationMode::CampaignShapeOfSilence) &&
            Owner == LocalPlayerId)
        {
            if (Type == EntityType::ScoutUnit)
            {
                MemoryBearerId = Spawned;
            }
            else if (Type == EntityType::Dropoff)
            {
                MigrationWaystoneId = Spawned;
            }
        }
        if (SelectedOperation ==
                EEchoesOperationMode::CampaignChoirAtLumeReach &&
            Owner == LocalPlayerId)
        {
            if (Type == EntityType::ScoutUnit &&
                ChoirAtLumeReachOruunId == 0)
            {
                ChoirAtLumeReachOruunId = Spawned;
            }
            else if (Type == EntityType::Dropoff &&
                     ChoirAtLumeReachWaystoneId == 0)
            {
                ChoirAtLumeReachWaystoneId = Spawned;
            }
        }
        if (SelectedOperation ==
                EEchoesOperationMode::CampaignNoNeutralLedger &&
            Owner == LocalPlayerId)
        {
            if (Type == EntityType::ScoutUnit && NoNeutralOruunId == 0)
            {
                NoNeutralOruunId = Spawned;
            }
            else if (Type == EntityType::Dropoff &&
                     NoNeutralWaystoneId == 0)
            {
                NoNeutralWaystoneId = Spawned;
            }
        }
        if (SelectedOperation ==
                EEchoesOperationMode::CampaignFutureThatWon &&
            Owner == LocalPlayerId && Type == EntityType::ScoutUnit)
        {
            if (FutureWonOruunId == 0)
            {
                FutureWonOruunId = Spawned;
            }
            else if (FutureWonVerifierId == 0)
            {
                FutureWonVerifierId = Spawned;
            }
        }
        if (SelectedOperation ==
                EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
            Owner == LocalPlayerId && Type == EntityType::ScoutUnit)
        {
            if (AssemblyOruunId == 0)
            {
                AssemblyOruunId = Spawned;
            }
            else if (AssemblyVerifierId == 0)
            {
                AssemblyVerifierId = Spawned;
            }
        }
        if (SelectedOperation ==
                EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
            Owner == LocalPlayerId)
        {
            if (Type == EntityType::Soldier &&
                SeveralVoicesPossibleVoiceId == 0)
            {
                SeveralVoicesPossibleVoiceId = Spawned;
            }
            else if (Type == EntityType::HeavyUnit &&
                     SeveralVoicesManifestVoiceId == 0)
            {
                SeveralVoicesManifestVoiceId = Spawned;
            }
            else if (Type == EntityType::ScoutUnit &&
                     SeveralVoicesNemeId == 0)
            {
                SeveralVoicesNemeId = Spawned;
            }
            else if (Type == EntityType::Barracks &&
                     SeveralVoicesResearchLoomId == 0)
            {
                SeveralVoicesResearchLoomId = Spawned;
            }
        }
        if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun)
        {
            if (Owner == LocalPlayerId)
            {
                if (Type == EntityType::Soldier &&
                    BrokenSunAccordVoiceId == 0)
                {
                    BrokenSunAccordVoiceId = Spawned;
                }
                else if (Type == EntityType::HeavyUnit &&
                         BrokenSunAccordHeavyId == 0)
                {
                    BrokenSunAccordHeavyId = Spawned;
                }
                else if (Type == EntityType::ScoutUnit &&
                         BrokenSunNemeId == 0)
                {
                    BrokenSunNemeId = Spawned;
                }
                else if (Type == EntityType::Worker &&
                         BrokenSunWorkerId == 0)
                {
                    BrokenSunWorkerId = Spawned;
                }
            }
            else if (Owner == 2 && Type == EntityType::ScoutUnit &&
                     BrokenSunMaraId == 0)
            {
                BrokenSunMaraId = Spawned;
            }
            else if (Owner == 2 && Type == EntityType::Worker &&
                     BrokenSunTalarId == 0)
            {
                BrokenSunTalarId = Spawned;
            }
            else if (Owner == 3 && Type == EntityType::ScoutUnit &&
                     BrokenSunOruunId == 0)
            {
                BrokenSunOruunId = Spawned;
            }
        }
        return Spawned;
    };

    EntityId KharuunSystemsMover = 0;
    if (bUseStressScenario)
    {
        constexpr int32 GridX[10] = {3, 9, 15, 21, 27, 36, 42, 48, 54, 60};
        constexpr int32 GridY[10] = {3, 8, 13, 18, 23, 28, 36, 43, 50, 57};
        constexpr uint8 Owners[4] = {0, 1, 2, 3};
        const Faction Factions[4] = {
            Faction::MeridianCompact,
            Faction::KharuunAssemblies,
            bUseSustainedStressScenario
                ? Faction::HollowChoir
                : Faction::KharuunAssemblies,
            Faction::MeridianCompact};
        constexpr int32 OffsetX[4] = {0, 1, 0, 1};
        constexpr int32 OffsetY[4] = {0, 0, 1, 1};
        for (int32 Team = 0; Team < 4; ++Team)
        {
            int32 TeamUnits = 0;
            for (int32 Row = 0; Row < 10; ++Row)
            {
                for (int32 Column = 0; Column < 10; ++Column)
                {
                    const EntityType Type = TeamUnits == 0
                                                ? EntityType::CommandCore
                                                : TeamUnits % 3 == 1
                                                      ? EntityType::Soldier
                                                      : TeamUnits % 3 == 2
                                                            ? EntityType::HeavyUnit
                                                            : EntityType::ScoutUnit;
                    const EntityId Spawned = SpawnUnit(
                        Owners[Team],
                        Factions[Team],
                        Type,
                        GridX[Column] + OffsetX[Team],
                        GridY[Row] + OffsetY[Team]);
                    if (bUseSustainedStressScenario)
                    {
                        if (Type == EntityType::CommandCore)
                        {
                            SustainedStressCommandCoreIds[Owners[Team]] = Spawned;
                        }
                        else
                        {
                            SustainedStressCombatEntityIds.Add(Spawned);
                            SustainedStressCombatOwners.Add(Owners[Team]);
                            SustainedStressCombatFactions.Add(Factions[Team]);
                            SustainedStressCombatTypes.Add(Type);
                            SustainedStressCombatSpawnPositions.Add(
                                Vec2::FromTiles(
                                    GridX[Column] + OffsetX[Team],
                                    GridY[Row] + OffsetY[Team]));
                        }
                    }
                    ++TeamUnits;
                }
            }
        }
        bSpawnSucceeded &=
            Simulation->SpawnFutureWell(Vec2::FromTiles(32, 32)) != 0;
    }
    else
    {
        const auto SpawnForce = [&SpawnUnit](
                                    uint8 Owner,
                                    Faction ForceFaction,
                                    bool bSouthwest)
        {
            if (bSouthwest)
            {
                SpawnUnit(Owner, ForceFaction, EntityType::CommandCore, 10, 10);
                SpawnUnit(Owner, ForceFaction, EntityType::Barracks, 14, 10);
                SpawnUnit(Owner, ForceFaction, EntityType::Dropoff, 6, 17);
                SpawnUnit(Owner, ForceFaction, EntityType::Worker, 8, 13);
                SpawnUnit(Owner, ForceFaction, EntityType::Worker, 11, 14);
                SpawnUnit(Owner, ForceFaction, EntityType::Worker, 14, 12);
                SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 8, 8);
                SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 12, 7);
                SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 16, 10);
                SpawnUnit(Owner, ForceFaction, EntityType::HeavyUnit, 7, 6);
                SpawnUnit(Owner, ForceFaction, EntityType::ScoutUnit, 15, 6);
                SpawnUnit(Owner, ForceFaction, EntityType::UtilityStructure, 6, 11);
                return;
            }
            SpawnUnit(Owner, ForceFaction, EntityType::CommandCore, 54, 54);
            SpawnUnit(Owner, ForceFaction, EntityType::Barracks, 50, 54);
            SpawnUnit(Owner, ForceFaction, EntityType::Dropoff, 58, 48);
            SpawnUnit(Owner, ForceFaction, EntityType::Worker, 51, 53);
            SpawnUnit(Owner, ForceFaction, EntityType::Worker, 54, 50);
            SpawnUnit(Owner, ForceFaction, EntityType::Worker, 57, 52);
            SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 50, 57);
            SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 54, 58);
            SpawnUnit(Owner, ForceFaction, EntityType::HeavyUnit, 57, 58);
            SpawnUnit(Owner, ForceFaction, EntityType::ScoutUnit, 49, 58);
            SpawnUnit(Owner, ForceFaction, EntityType::UtilityStructure, 58, 53);
        };
        if (bUseNetworkMatchSmoke)
        {
            const EntityId AuthorityCore = SpawnUnit(
                LocalPlayerId,
                ScenarioLocalFaction,
                EntityType::CommandCore,
                18,
                18);
            const EntityId RemoteCore = SpawnUnit(
                OpponentPlayerId,
                ScenarioOpponentFaction,
                EntityType::CommandCore,
                54,
                54);
            constexpr int32 RemoteAttackerCount = 24;
            EntityId FirstRemoteAttacker = 0;
            EntityId LastRemoteAttacker = 0;
            for (int32 Index = 0; Index < RemoteAttackerCount; ++Index)
            {
                const EntityId Attacker = SpawnUnit(
                    OpponentPlayerId,
                    ScenarioOpponentFaction,
                    EntityType::Soldier,
                    21,
                    18);
                FirstRemoteAttacker = FirstRemoteAttacker == 0
                    ? Attacker
                    : FirstRemoteAttacker;
                LastRemoteAttacker = Attacker;
            }
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NETWORK_MATCH_FIXTURE] authorityCore=%u remoteCore=%u remoteAttackers=%d firstRemoteAttacker=%u lastRemoteAttacker=%u controlledNonshipping=true ordinaryCombatResolution=true"),
                AuthorityCore,
                RemoteCore,
                RemoteAttackerCount,
                FirstRemoteAttacker,
                LastRemoteAttacker);
        }
        else if (bUsePointerCombatGuardPresentation)
        {
            const EntityId PointerCore = SpawnUnit(
                LocalPlayerId,
                ScenarioLocalFaction,
                EntityType::CommandCore,
                18,
                18);
            const EntityId PointerProtected = SpawnUnit(
                LocalPlayerId,
                ScenarioLocalFaction,
                EntityType::Worker,
                21,
                20);
            const EntityId PointerDefender = SpawnUnit(
                LocalPlayerId,
                ScenarioLocalFaction,
                EntityType::HeavyUnit,
                20,
                23);
            const EntityId PointerHostile = SpawnUnit(
                OpponentPlayerId,
                ScenarioOpponentFaction,
                EntityType::Soldier,
                28,
                20);
            const EntityId PointerOpponentCore = SpawnUnit(
                OpponentPlayerId,
                ScenarioOpponentFaction,
                EntityType::CommandCore,
                54,
                54);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_POINTER_COMBAT_GUARD_FIXTURE] localCore=%u defender=%u protected=%u hostile=%u opponentCore=%u authoritativeCommands=true controlledNonshipping=true"),
                PointerCore,
                PointerDefender,
                PointerProtected,
                PointerHostile,
                PointerOpponentCore);
        }
        else
        {
            SpawnForce(LocalPlayerId, ScenarioLocalFaction, true);
            SpawnForce(OpponentPlayerId, ScenarioOpponentFaction, false);
        }

        if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun)
        {
            const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
            BrokenSunMaraId = SpawnUnit(
                2,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                Plan.MaraAccordSite.x.FloorToInt() - 2,
                Plan.MaraAccordSite.y.FloorToInt());
            BrokenSunTalarId = SpawnUnit(
                2,
                Faction::MeridianCompact,
                EntityType::Worker,
                Plan.TalarPublicRecordSite.x.FloorToInt(),
                Plan.TalarPublicRecordSite.y.FloorToInt());
            BrokenSunOruunId = SpawnUnit(
                3,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                Plan.OruunAccordSite.x.FloorToInt() + 2,
                Plan.OruunAccordSite.y.FloorToInt());
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_BROKEN_SUN_WITNESSES_SPAWNED] mara=%u oruun=%u talar=%u neme=%u neutralWitnesses=true mixedFactionCommand=false success=%s"),
                BrokenSunMaraId,
                BrokenSunOruunId,
                BrokenSunTalarId,
                BrokenSunNemeId,
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
                EEchoesOperationMode::CampaignCityReserve ||
            SelectedOperation ==
                EEchoesOperationMode::CampaignReserveAuthority)
        {
            const echoes::sim::Vec2 LifeSupportSite =
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::LifeSupport);
            const echoes::sim::Vec2 TransitSite =
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Transit);
            const echoes::sim::Vec2 ArchiveSite =
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Archive);
            LifeSupportDistrictId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                LifeSupportSite.x.FloorToInt(),
                LifeSupportSite.y.FloorToInt());
            TransitDistrictId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                TransitSite.x.FloorToInt(),
                TransitSite.y.FloorToInt());
            ArchiveDistrictId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                ArchiveSite.x.FloorToInt(),
                ArchiveSite.y.FloorToInt());
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority)
        {
            const FEchoesReserveAuthorityPlan Plan =
                GetReserveAuthorityPlan();
            ReserveAuthorityMaraId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                18,
                20);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESERVE_AUTHORITY_SPAWN] branch=%s mara=%u districts=%u,%u,%u authority=(%d,%d) success=%s"),
                Plan.StableName,
                ReserveAuthorityMaraId,
                LifeSupportDistrictId,
                TransitDistrictId,
                ArchiveDistrictId,
                Plan.AuthoritySite.x.FloorToInt(),
                Plan.AuthoritySite.y.FloorToInt(),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            const FEchoesTermsOfContinuancePlan Plan =
                GetTermsOfContinuancePlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_TERMS_OF_CONTINUANCE_SPAWN] branch=%s begin=true"),
                Plan.StableName);
            MeridianContinuanceRelayId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                Plan.MeridianRelaySite.x.FloorToInt(),
                Plan.MeridianRelaySite.y.FloorToInt());
            KharuunContinuanceSpineId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                Plan.KharuunSpineSite.x.FloorToInt(),
                Plan.KharuunSpineSite.y.FloorToInt());
            MeridianContinuanceWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                20,
                24);
            KharuunContinuanceWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                23,
                24);
            const FIntPoint TreatyLinks[] = {
                {18, 10}, {24, 15}, {29, 20}, {29, 36}, {29, 40}};
            for (const FIntPoint& Link : TreatyLinks)
            {
                SpawnUnit(
                    LocalPlayerId,
                    Faction::MeridianCompact,
                    EntityType::Dropoff,
                    Link.X,
                    Link.Y);
            }
            for (int32 Index = 0; Index < 2; ++Index)
            {
                SpawnUnit(
                    OpponentPlayerId,
                    ScenarioOpponentFaction,
                    EntityType::ScoutUnit,
                    48 + Index * 3,
                    56);
            }
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_TERMS_OF_CONTINUANCE_SPAWN] meridianRelay=%u kharuunSpine=%u meridianWitness=%u kharuunWitness=%u success=%s"),
                MeridianContinuanceRelayId,
                KharuunContinuanceSpineId,
                MeridianContinuanceWitnessId,
                KharuunContinuanceWitnessId,
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            const FEchoesNamesWithoutBirthsPlan Plan =
                GetNamesWithoutBirthsPlan();
            CensusArchiveId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                Plan.CensusSite.x.FloorToInt(),
                Plan.CensusSite.y.FloorToInt());
            TalarId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                18,
                20);
            FirstCivilianId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::Worker,
                20,
                24);
            SecondCivilianId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::Worker,
                23,
                24);
            const FIntPoint CommonArchiveLinks[] = {{18, 10}, {24, 15}};
            for (const FIntPoint& Link : CommonArchiveLinks)
            {
                SpawnUnit(
                    LocalPlayerId,
                    Faction::MeridianCompact,
                    EntityType::Dropoff,
                    Link.X,
                    Link.Y);
            }
            if (Plan.PriorChoice == FutureWellChoice::Reshape)
            {
                const FIntPoint EasternArchiveLinks[] = {{31, 17}, {38, 19}};
                for (const FIntPoint& Link : EasternArchiveLinks)
                {
                    SpawnUnit(
                        LocalPlayerId,
                        Faction::MeridianCompact,
                        EntityType::Dropoff,
                        Link.X,
                        Link.Y);
                }
            }
            for (int32 Index = 0; Index < 3; ++Index)
            {
                SpawnUnit(
                    OpponentPlayerId,
                    ScenarioOpponentFaction,
                    EntityType::ScoutUnit,
                    46 + Index * 3,
                    54);
            }
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_SPAWN] branch=%s talar=%u archive=%u civilianA=%u civilianB=%u requiredLink=%d,%d success=%s"),
                Plan.StableName,
                TalarId,
                CensusArchiveId,
                FirstCivilianId,
                SecondCivilianId,
                Plan.PowerLinkSite.x.FloorToInt(),
                Plan.PowerLinkSite.y.FloorToInt(),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence)
        {
            const FEchoesShapeOfSilencePlan Plan =
                GetShapeOfSilencePlan();
            OruunId = MemoryBearerId;
            FirstMemoryWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                20,
                24);
            SecondMemoryWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                23,
                24);
            MemoryBearerId = OruunId;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_OF_SILENCE_SPAWN] branch=%s oruun=%u witnessA=%u witnessB=%u waystone=%u success=%s"),
                Plan.StableName,
                OruunId,
                FirstMemoryWitnessId,
                SecondMemoryWitnessId,
                MigrationWaystoneId,
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs)
        {
            const FEchoesShapeBesideUsPlan Plan = GetShapeBesideUsPlan();
            ShapeBesideUsTalarId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                18,
                20);
            FirstStateWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::Worker,
                20,
                24);
            SecondStateWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::Soldier,
                23,
                24);
            for (int32 Index = 0; Index < 3; ++Index)
            {
                SpawnUnit(
                    OpponentPlayerId,
                    ScenarioOpponentFaction,
                    EntityType::ScoutUnit,
                    46 + Index * 3,
                    54);
            }
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_BESIDE_US_SPAWN] branch=%s talar=%u witnessA=%u witnessB=%u relay=(%d,%d) success=%s"),
                Plan.StableName,
                ShapeBesideUsTalarId,
                FirstStateWitnessId,
                SecondStateWitnessId,
                Plan.EchoRelaySite.x.FloorToInt(),
                Plan.EchoRelaySite.y.FloorToInt(),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            const FEchoesNoNeutralLedgerPlan Plan =
                GetNoNeutralLedgerPlan();
            NoNeutralLedgerWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                18,
                22);
            NoNeutralFirstDistrictInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.FirstDistrictSite);
            NoNeutralSecondDistrictInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.SecondDistrictSite);
            NoNeutralMeridianEvidenceInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.MeridianEvidenceSite);
            NoNeutralKharuunEvidenceInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::KharuunAssemblies,
                    Plan.KharuunEvidenceSite);
            bSpawnSucceeded &=
                NoNeutralFirstDistrictInterfaceId != 0 &&
                NoNeutralSecondDistrictInterfaceId != 0 &&
                NoNeutralMeridianEvidenceInterfaceId != 0 &&
                NoNeutralKharuunEvidenceInterfaceId != 0;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NO_NEUTRAL_LEDGER_SPAWN] planKey=%u oruun=%u waystone=%u witness=%u districtInterfaces=%u:%u evidenceInterfaces=%u:%u route=(%d,%d) districts=(%d,%d):(%d,%d) evidence=(%d,%d):(%d,%d) protocol=%u publicInterfacesNeutral=true success=%s"),
                Plan.StablePlanKey,
                NoNeutralOruunId,
                NoNeutralWaystoneId,
                NoNeutralLedgerWitnessId,
                NoNeutralFirstDistrictInterfaceId,
                NoNeutralSecondDistrictInterfaceId,
                NoNeutralMeridianEvidenceInterfaceId,
                NoNeutralKharuunEvidenceInterfaceId,
                Plan.RouteSite.x.FloorToInt(),
                Plan.RouteSite.y.FloorToInt(),
                Plan.FirstDistrictSite.x.FloorToInt(),
                Plan.FirstDistrictSite.y.FloorToInt(),
                Plan.SecondDistrictSite.x.FloorToInt(),
                Plan.SecondDistrictSite.y.FloorToInt(),
                Plan.MeridianEvidenceSite.x.FloorToInt(),
                Plan.MeridianEvidenceSite.y.FloorToInt(),
                Plan.KharuunEvidenceSite.x.FloorToInt(),
                Plan.KharuunEvidenceSite.y.FloorToInt(),
                static_cast<uint8>(Plan.LumeProtocol),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon)
        {
            const FEchoesFutureThatWonPlan Plan = GetFutureThatWonPlan();
            FutureWonVerifierId = SpawnUnit(
                LocalPlayerId,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                18,
                22);
            FutureWonFirstDistrictInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.FirstDistrictInputSite);
            FutureWonSecondDistrictInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.SecondDistrictInputSite);
            FutureWonMeridianReadbackInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.MeridianReadbackSite);
            FutureWonKharuunReadbackInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::KharuunAssemblies,
                    Plan.KharuunReadbackSite);
            FutureWonDemonstratorInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.RestorationDemonstratorSite);
            bSpawnSucceeded &=
                FutureWonFirstDistrictInterfaceId != 0 &&
                FutureWonSecondDistrictInterfaceId != 0 &&
                FutureWonMeridianReadbackInterfaceId != 0 &&
                FutureWonKharuunReadbackInterfaceId != 0 &&
                FutureWonDemonstratorInterfaceId != 0;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_FUTURE_THAT_WON_SPAWN] planKey=%u oruun=%u verifier=%u districtReadbacks=%u:%u evidenceReadbacks=%u:%u demonstrator=%u protocol=%u publicInterfacesNeutral=true success=%s"),
                Plan.StablePlanKey,
                FutureWonOruunId,
                FutureWonVerifierId,
                FutureWonFirstDistrictInterfaceId,
                FutureWonSecondDistrictInterfaceId,
                FutureWonMeridianReadbackInterfaceId,
                FutureWonKharuunReadbackInterfaceId,
                FutureWonDemonstratorInterfaceId,
                static_cast<uint8>(Plan.RecordedProtocol),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            const FEchoesAssemblyOfTheMissingPlan Plan =
                GetAssemblyOfTheMissingPlan();
            AssemblyVerifierId = SpawnUnit(
                LocalPlayerId,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                18,
                22);
            AssemblyMeridianPublicRecordInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.MeridianPublicRecordSite);
            AssemblyKharuunPublicRecordInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::KharuunAssemblies,
                    Plan.KharuunPublicRecordSite);
            AssemblyCrownfallIndexInterfaceId =
                Simulation->SpawnPublicInterface(
                    Faction::MeridianCompact,
                    Plan.CrownfallIndexSite);
            bSpawnSucceeded &=
                AssemblyMeridianPublicRecordInterfaceId != 0 &&
                AssemblyKharuunPublicRecordInterfaceId != 0 &&
                AssemblyCrownfallIndexInterfaceId != 0;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_ASSEMBLY_OF_THE_MISSING_SPAWN] planKey=%u oruun=%u verifier=%u publicRecords=%u:%u crownfallIndex=%u protocol=%u publicInterfacesNeutral=true success=%s"),
                Plan.StablePlanKey,
                AssemblyOruunId,
                AssemblyVerifierId,
                AssemblyMeridianPublicRecordInterfaceId,
                AssemblyKharuunPublicRecordInterfaceId,
                AssemblyCrownfallIndexInterfaceId,
                static_cast<uint8>(Plan.RecordedProtocol),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            const FEchoesSeveralVoicesOneCommandPlan Plan =
                GetSeveralVoicesOneCommandPlan();
            bSpawnSucceeded &= SeveralVoicesPossibleVoiceId != 0 &&
                SeveralVoicesManifestVoiceId != 0 &&
                SeveralVoicesNemeId != 0 &&
                SeveralVoicesResearchLoomId != 0;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SEVERAL_VOICES_ONE_COMMAND_SPAWN] planKey=%u possibleVoice=%u manifestVoice=%u neme=%u researchLoom=%u possibleSite=(%d,%d) manifestSite=(%d,%d) nemeSite=(%d,%d) anchorSite=(%d,%d) protocol=%u localAuthority=HollowChoir incompatibleStates=true success=%s"),
                Plan.StablePlanKey,
                SeveralVoicesPossibleVoiceId,
                SeveralVoicesManifestVoiceId,
                SeveralVoicesNemeId,
                SeveralVoicesResearchLoomId,
                Plan.PossibleVoiceSite.x.FloorToInt(),
                Plan.PossibleVoiceSite.y.FloorToInt(),
                Plan.ManifestVoiceSite.x.FloorToInt(),
                Plan.ManifestVoiceSite.y.FloorToInt(),
                Plan.NemeCommandSite.x.FloorToInt(),
                Plan.NemeCommandSite.y.FloorToInt(),
                Plan.CrisisAnchorSite.x.FloorToInt(),
                Plan.CrisisAnchorSite.y.FloorToInt(),
                static_cast<uint8>(Plan.RecordedProtocol),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (bUseResearchInterruptionPresentation)
        {
            constexpr int32 InterruptionAttackerCount = 32;
            for (int32 Index = 0; Index < InterruptionAttackerCount; ++Index)
            {
                SpawnUnit(
                    2,
                    ScenarioOpponentFaction,
                    EntityType::Soldier,
                    17,
                    10);
            }
        }
        if (bUseKharuunSystemsPresentation)
        {
            KharuunSystemsMover = SpawnUnit(
                2,
                Faction::MeridianCompact,
                EntityType::Soldier,
                31,
                0);
        }

        const TArray<FIntPoint> MatterNodeTiles = bLumeReach
            ? TArray<FIntPoint>{
                  {16, 16}, {21, 13}, {26, 24}, {38, 24},
                  {24, 48}, {40, 48}, {48, 42}, {52, 45}}
            : TArray<FIntPoint>{
                  SelectedOperation == EEchoesOperationMode::CampaignCityReserve
                      ? FIntPoint{17, 27}
                      : FIntPoint{16, 16},
                  {21, 13}, {25, 28}, {33, 22},
                  {31, 43}, {43, 36}, {47, 50}, {52, 45}};
        for (const FIntPoint& Tile : MatterNodeTiles)
        {
            bSpawnSucceeded &=
                Simulation->SpawnResourceNode(Vec2::FromTiles(Tile.X, Tile.Y), 1600) != 0;
        }
        if (bLumeReach)
        {
            if (SelectedOperation ==
                EEchoesOperationMode::CampaignAssemblyOfTheMissing ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignTheBrokenSun)
            {
                // Missions 13 through 15 retain the recorded protocol as
                // provenance; none exposes another three-way Well choice.
            }
            else if (SelectedOperation ==
                EEchoesOperationMode::CampaignFutureThatWon)
            {
                const FEchoesFutureThatWonPlan Plan =
                    GetFutureThatWonPlan();
                FutureWonWellId = Simulation->SpawnFutureWell(
                    Plan.FutureWellSite);
                bSpawnSucceeded &= FutureWonWellId != 0;
            }
            else if (SelectedOperation ==
                EEchoesOperationMode::CampaignNoNeutralLedger)
            {
                const FEchoesNoNeutralLedgerPlan Plan =
                    GetNoNeutralLedgerPlan();
                NoNeutralWellId = Simulation->SpawnFutureWell(
                    Plan.FutureWellSite);
                bSpawnSucceeded &= NoNeutralWellId != 0;
            }
            else
            {
                const FEchoesChoirAtLumeReachPlan Plan =
                    GetChoirAtLumeReachPlan();
                ChoirAtLumeReachWellId = Simulation->SpawnFutureWell(
                    Plan.FutureWellSite);
                bSpawnSucceeded &= ChoirAtLumeReachWellId != 0;
            }
        }
        else
        {
            bSpawnSucceeded &=
                Simulation->SpawnFutureWell(Vec2::FromTiles(32, 32)) != 0;
        }
    }

    if (!bSpawnSucceeded)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_SCENARIO_FAILED] At least one required prototype entity could not be spawned."));
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun &&
        (BrokenSunAccordVoiceId == 0 || BrokenSunAccordHeavyId == 0 ||
         BrokenSunNemeId == 0 || BrokenSunWorkerId == 0 ||
         BrokenSunMaraId == 0 || BrokenSunOruunId == 0 ||
         BrokenSunTalarId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_THE_BROKEN_SUN_INIT_FAILED] reason=required command force or named witness unavailable voice=%u heavy=%u neme=%u worker=%u mara=%u oruun=%u talar=%u"),
            BrokenSunAccordVoiceId,
            BrokenSunAccordHeavyId,
            BrokenSunNemeId,
            BrokenSunWorkerId,
            BrokenSunMaraId,
            BrokenSunOruunId,
            BrokenSunTalarId);
        Simulation.Reset();
        return false;
    }

    Simulation->CaptureReplayBaseline();
    if (bUseSustainedStressScenario)
    {
        // Replacement is maintained by the non-shipping fixture rather than
        // SimCore commands, so exporting a superficially valid replay would
        // misrepresent what can be reproduced deterministically.
        Simulation->DisableReplayExport();
    }
    ProloguePresentationWorkerId = 0;
    ProloguePresentationWellId = 0;
    if (bUsePrologueCompletionPresentation)
    {
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == LocalPlayerId &&
                Entity.type == EntityType::Worker &&
                ProloguePresentationWorkerId == 0)
            {
                ProloguePresentationWorkerId = Entity.id;
            }
            if (Entity.type == EntityType::FutureWell)
            {
                ProloguePresentationWellId = Entity.id;
            }
        }
        if (ArchiveCarrierId == 0 || ProloguePresentationWorkerId == 0 ||
            ProloguePresentationWellId == 0)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_FAILED] reason=fixture entities unavailable carrier=%u worker=%u well=%u"),
                ArchiveCarrierId,
                ProloguePresentationWorkerId,
                ProloguePresentationWellId);
            Simulation.Reset();
            return false;
        }
    }
    ResearchPresentationTechnology = echoes::sim::ResearchType::None;
    if (bUseAnyResearchPresentation)
    {
        uint32 ProducerId = 0;
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == LocalPlayerId &&
                Entity.type == EntityType::Barracks)
            {
                ProducerId = Entity.id;
                break;
            }
        }
        if (ProducerId == 0)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] producer=0 reason=local production structure unavailable"));
            Simulation.Reset();
            return false;
        }
        ResearchPresentationTechnology =
            echoes::presentation::TechnologyProfile(ScenarioLocalFaction)
                .TierOne;
        echoes::sim::Command Command{};
        Command.executeTick = Simulation->CurrentTick() + 1;
        Command.player = LocalPlayerId;
        Command.sequence = 1;
        Command.type = echoes::sim::CommandType::Research;
        Command.actor = ProducerId;
        Command.researchType = ResearchPresentationTechnology;
        std::string Rejection;
        if (!Simulation->QueueCommand(Command, &Rejection))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] producer=%u reason=%s"),
                ProducerId,
                UTF8_TO_TCHAR(Rejection.c_str()));
            Simulation.Reset();
            ResearchPresentationTechnology =
                echoes::sim::ResearchType::None;
            return false;
        }
        if (bUseResearchInterruptionPresentation)
        {
            uint64 InterruptionSequence = 1;
            int32 QueuedAttackers = 0;
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (Entity.owner != 2 || Entity.type != EntityType::Soldier)
                {
                    continue;
                }
                echoes::sim::Command Attack{};
                Attack.executeTick = 60;
                Attack.player = 2;
                Attack.sequence = InterruptionSequence++;
                Attack.type = echoes::sim::CommandType::Attack;
                Attack.actor = Entity.id;
                Attack.target = ProducerId;
                if (!Simulation->QueueCommand(Attack, &Rejection))
                {
                    UE_LOG(
                        LogEchoes,
                        Error,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] attacker=%u reason=%s"),
                        Entity.id,
                        UTF8_TO_TCHAR(Rejection.c_str()));
                    Simulation.Reset();
                    ResearchPresentationTechnology =
                        echoes::sim::ResearchType::None;
                    return false;
                }
                ++QueuedAttackers;
            }
            if (QueuedAttackers != 32)
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] interruptionAttackers=%d expected=32"),
                    QueuedAttackers);
                Simulation.Reset();
                ResearchPresentationTechnology =
                    echoes::sim::ResearchType::None;
                return false;
            }
        }
    }
    if (bUseKharuunSystemsPresentation)
    {
        uint32 WaystoneId = 0;
        uint32 BasinId = 0;
        uint32 WarformId = 0;
        uint32 CairnbackId = 0;
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner != LocalPlayerId)
            {
                continue;
            }
            WaystoneId = Entity.type == EntityType::Dropoff
                             ? Entity.id
                             : WaystoneId;
            BasinId = Entity.type == EntityType::Barracks
                          ? Entity.id
                          : BasinId;
            CairnbackId = Entity.type == EntityType::HeavyUnit
                              ? Entity.id
                              : CairnbackId;
        }
        const echoes::sim::Entity* Basin = Simulation->FindEntity(BasinId);
        uint64 NearestWarformDistance = TNumericLimits<uint64>::Max();
        if (Basin != nullptr)
        {
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (Entity.owner != LocalPlayerId ||
                    Entity.type != EntityType::Soldier)
                {
                    continue;
                }
                const int64 DeltaX =
                    static_cast<int64>(Entity.position.x.Raw()) -
                    Basin->position.x.Raw();
                const int64 DeltaY =
                    static_cast<int64>(Entity.position.y.Raw()) -
                    Basin->position.y.Raw();
                const uint64 Distance = static_cast<uint64>(
                    DeltaX * DeltaX + DeltaY * DeltaY);
                if (Distance < NearestWarformDistance)
                {
                    NearestWarformDistance = Distance;
                    WarformId = Entity.id;
                }
            }
        }
        if (WaystoneId == 0 || BasinId == 0 || WarformId == 0 ||
            CairnbackId == 0 || KharuunSystemsMover == 0)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_FAILED] reason=fixture entities unavailable"));
            Simulation.Reset();
            return false;
        }
        const auto QueueFixtureCommand = [this](
                                             echoes::sim::Command Command,
                                             const TCHAR* Label)
        {
            std::string Rejection;
            if (Simulation->QueueCommand(Command, &Rejection))
            {
                return true;
            }
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_FAILED] command=%s reason=%s"),
                Label,
                UTF8_TO_TCHAR(Rejection.c_str()));
            return false;
        };

        echoes::sim::Command Waystone{};
        Waystone.executeTick = 1;
        Waystone.player = LocalPlayerId;
        Waystone.sequence = 1;
        Waystone.type = echoes::sim::CommandType::ToggleWaystoneRoot;
        Waystone.actor = WaystoneId;

        echoes::sim::Command Adapt{};
        Adapt.executeTick = 1;
        Adapt.player = LocalPlayerId;
        Adapt.sequence = 2;
        Adapt.type = echoes::sim::CommandType::AdaptWarform;
        Adapt.actor = WarformId;
        Adapt.target = BasinId;
        Adapt.warformAdaptation = echoes::sim::WarformAdaptation::Carapace;

        echoes::sim::Command Cover{};
        Cover.executeTick = 1;
        Cover.player = LocalPlayerId;
        Cover.sequence = 3;
        Cover.type = echoes::sim::CommandType::RaiseMineralCover;
        Cover.actor = CairnbackId;
        Cover.position = Vec2::FromTiles(7, 4);

        echoes::sim::Command Move{};
        Move.executeTick = 1;
        Move.player = 2;
        Move.sequence = 1;
        Move.type = echoes::sim::CommandType::Move;
        Move.actor = KharuunSystemsMover;
        Move.position = Vec2::FromTiles(30, 0);

        if (!QueueFixtureCommand(Waystone, TEXT("waystone")) ||
            !QueueFixtureCommand(Adapt, TEXT("carapace")) ||
            !QueueFixtureCommand(Cover, TEXT("mineral_cover")) ||
            !QueueFixtureCommand(Move, TEXT("hidden_movement")))
        {
            Simulation.Reset();
            return false;
        }
    }
    int32 StressAttackMoveCommands = 0;
    if (bUseStressScenario)
    {
        constexpr Vec2 TeamDestinations[4] = {
            Vec2::FromTiles(46, 46),
            Vec2::FromTiles(18, 46),
            Vec2::FromTiles(46, 18),
            Vec2::FromTiles(18, 18)};
        uint64 TeamSequences[4] = {1, 1, 1, 1};
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner >= 4 ||
                (Entity.type != EntityType::Soldier &&
                 Entity.type != EntityType::HeavyUnit &&
                 Entity.type != EntityType::ScoutUnit))
            {
                continue;
            }
            echoes::sim::Command Command;
            Command.executeTick = Simulation->CurrentTick() + 1;
            Command.player = Entity.owner;
            Command.sequence = TeamSequences[Entity.owner]++;
            Command.type = echoes::sim::CommandType::AttackMove;
            Command.actor = Entity.id;
            Command.position = TeamDestinations[Entity.owner];
            std::string Rejection;
            if (!Simulation->QueueCommand(Command, &Rejection))
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_STRESS_ORDER_FAILED] actor=%u owner=%u reason=%s"),
                    Entity.id,
                    Entity.owner,
                    UTF8_TO_TCHAR(Rejection.c_str()));
                Simulation.Reset();
                return false;
            }
            ++StressAttackMoveCommands;
        }
        if (StressAttackMoveCommands != 396)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_STRESS_ORDER_COUNT_FAILED] attackMove=%d expected=396"),
                StressAttackMoveCommands);
            Simulation.Reset();
            return false;
        }
    }
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = bUseKharuunSystemsPresentation
                                    ? 4
                                    : bUseAnyResearchPresentation ? 2 : 1;
    bLoggedFirstTick = false;
    bLoggedStressCombat = false;
    bLoggedAiExpansion = false;
    bLoggedAiRetreat = false;
    bLoggedAiPlayerView = false;
    bLoggedAiAdaptation = false;
    bLoggedAiMineralCover = false;
    bLoggedAiVibrationResponse = false;
    bResearchPresentationScenario = bUseResearchPresentation;
    bResearchInterruptionPresentationScenario =
        bUseResearchInterruptionPresentation;
    bKharuunSystemsPresentationScenario =
        bUseKharuunSystemsPresentation;
    bPrologueCompletionPresentationScenario =
        bUsePrologueCompletionPresentation;
    bPointerCombatGuardPresentationScenario =
        bUsePointerCombatGuardPresentation;
    bLoggedResearchPresentationActive = false;
    bLoggedResearchPresentationComplete = false;
    bLoggedResearchPresentationInterrupted = false;
    bLoggedKharuunSystemsPresentation = false;
    PrologueCompletionPresentationStage = 0;
    bSimulationPaused = false;
    bMatchResultReported = false;
    bStressScenario = bUseStressScenario;
    bSustainedStressScenario = bUseSustainedStressScenario;
    bSustainedStressFailed = false;
    bSustainedStressTimingReady = false;
    bSustainedStressQualificationLogged = false;
    SustainedStressFailureCode.Reset();
    SustainedStressIntervalDamage = 0;
    SustainedStressIntervalCombatLosses = 0;
    SustainedStressCumulativeCombatLosses = 0;
    SustainedStressIntervalReplacements = 0;
    SustainedStressCumulativeReplacements = 0;
    SustainedStressIntervalOrderRenewals = 0;
    SustainedStressCumulativeOrderRenewals = 0;
    SustainedStressLastActivityTick = 0;
    SustainedStressLastHeartbeatTick = 0;
    SustainedStressLastHeartbeatWallMs = 0;
    SustainedStressStartupStableFrames = 0;
    SustainedStressStartupStableSeconds = 0.0;
    SustainedStressRenewalCursorByPlayer.fill(0);
    SustainedStressReadyWallSeconds = 0.0;
    if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
        ArchiveCarrierId == 0)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PROLOGUE_INIT_FAILED] reason=archive carrier unavailable"));
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        (MemoryBearerId == 0 || MigrationWaystoneId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SEVEN_ACCOUNTS_INIT_FAILED] reason=mission entities unavailable bearer=%u waystone=%u"),
            MemoryBearerId,
            MigrationWaystoneId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        (MemoryBearerId == 0 || MigrationWaystoneId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_UNBURIED_ROAD_INIT_FAILED] reason=mission entities unavailable bearer=%u waystone=%u"),
            MemoryBearerId,
            MigrationWaystoneId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve &&
        (LifeSupportDistrictId == 0 || TransitDistrictId == 0 ||
         ArchiveDistrictId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CITY_RESERVE_INIT_FAILED] reason=district entities unavailable life=%u transit=%u archive=%u"),
            LifeSupportDistrictId,
            TransitDistrictId,
            ArchiveDistrictId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        (MeridianContinuanceRelayId == 0 ||
         KharuunContinuanceSpineId == 0 ||
         MeridianContinuanceWitnessId == 0 ||
         KharuunContinuanceWitnessId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_TERMS_OF_CONTINUANCE_INIT_FAILED] reason=mission entities unavailable meridianRelay=%u kharuunSpine=%u meridianWitness=%u kharuunWitness=%u"),
            MeridianContinuanceRelayId,
            KharuunContinuanceSpineId,
            MeridianContinuanceWitnessId,
            KharuunContinuanceWitnessId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        (TalarId == 0 || CensusArchiveId == 0 || FirstCivilianId == 0 ||
         SecondCivilianId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_INIT_FAILED] reason=mission entities unavailable talar=%u archive=%u civilianA=%u civilianB=%u"),
            TalarId,
            CensusArchiveId,
            FirstCivilianId,
            SecondCivilianId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        (OruunId == 0 || FirstMemoryWitnessId == 0 ||
         SecondMemoryWitnessId == 0 || MigrationWaystoneId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SHAPE_OF_SILENCE_INIT_FAILED] reason=mission entities unavailable oruun=%u witnessA=%u witnessB=%u waystone=%u"),
            OruunId,
            FirstMemoryWitnessId,
            SecondMemoryWitnessId,
            MigrationWaystoneId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs &&
        (ShapeBesideUsTalarId == 0 || FirstStateWitnessId == 0 ||
         SecondStateWitnessId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SHAPE_BESIDE_US_INIT_FAILED] reason=mission entities unavailable talar=%u witnessA=%u witnessB=%u"),
            ShapeBesideUsTalarId,
            FirstStateWitnessId,
            SecondStateWitnessId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority &&
        (ReserveAuthorityMaraId == 0 || LifeSupportDistrictId == 0 ||
         TransitDistrictId == 0 || ArchiveDistrictId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_RESERVE_AUTHORITY_INIT_FAILED] reason=mission entities unavailable mara=%u life=%u transit=%u archive=%u"),
            ReserveAuthorityMaraId,
            LifeSupportDistrictId,
            TransitDistrictId,
            ArchiveDistrictId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach &&
        (ChoirAtLumeReachOruunId == 0 ||
         ChoirAtLumeReachWaystoneId == 0 ||
         ChoirAtLumeReachWellId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CHOIR_AT_LUME_REACH_INIT_FAILED] reason=mission entities unavailable oruun=%u waystone=%u well=%u"),
            ChoirAtLumeReachOruunId,
            ChoirAtLumeReachWaystoneId,
            ChoirAtLumeReachWellId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger &&
        (NoNeutralOruunId == 0 || NoNeutralWaystoneId == 0 ||
         NoNeutralLedgerWitnessId == 0 ||
         NoNeutralFirstDistrictInterfaceId == 0 ||
         NoNeutralSecondDistrictInterfaceId == 0 ||
         NoNeutralMeridianEvidenceInterfaceId == 0 ||
         NoNeutralKharuunEvidenceInterfaceId == 0 ||
         NoNeutralWellId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NO_NEUTRAL_LEDGER_INIT_FAILED] reason=mission entities unavailable oruun=%u waystone=%u witness=%u districtInterfaces=%u:%u evidenceInterfaces=%u:%u well=%u"),
            NoNeutralOruunId,
            NoNeutralWaystoneId,
            NoNeutralLedgerWitnessId,
            NoNeutralFirstDistrictInterfaceId,
            NoNeutralSecondDistrictInterfaceId,
            NoNeutralMeridianEvidenceInterfaceId,
            NoNeutralKharuunEvidenceInterfaceId,
            NoNeutralWellId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon &&
        (FutureWonOruunId == 0 || FutureWonVerifierId == 0 ||
         FutureWonFirstDistrictInterfaceId == 0 ||
         FutureWonSecondDistrictInterfaceId == 0 ||
         FutureWonMeridianReadbackInterfaceId == 0 ||
         FutureWonKharuunReadbackInterfaceId == 0 ||
         FutureWonDemonstratorInterfaceId == 0 || FutureWonWellId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_FUTURE_THAT_WON_INIT_FAILED] reason=mission entities unavailable oruun=%u verifier=%u districts=%u:%u evidence=%u:%u demonstrator=%u well=%u"),
            FutureWonOruunId,
            FutureWonVerifierId,
            FutureWonFirstDistrictInterfaceId,
            FutureWonSecondDistrictInterfaceId,
            FutureWonMeridianReadbackInterfaceId,
            FutureWonKharuunReadbackInterfaceId,
            FutureWonDemonstratorInterfaceId,
            FutureWonWellId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
        (AssemblyOruunId == 0 || AssemblyVerifierId == 0 ||
         AssemblyMeridianPublicRecordInterfaceId == 0 ||
         AssemblyKharuunPublicRecordInterfaceId == 0 ||
         AssemblyCrownfallIndexInterfaceId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_ASSEMBLY_OF_THE_MISSING_INIT_FAILED] reason=mission entities unavailable oruun=%u verifier=%u publicRecords=%u:%u crownfallIndex=%u"),
            AssemblyOruunId,
            AssemblyVerifierId,
            AssemblyMeridianPublicRecordInterfaceId,
            AssemblyKharuunPublicRecordInterfaceId,
            AssemblyCrownfallIndexInterfaceId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        (SeveralVoicesPossibleVoiceId == 0 ||
         SeveralVoicesManifestVoiceId == 0 ||
         SeveralVoicesNemeId == 0 ||
         SeveralVoicesResearchLoomId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SEVERAL_VOICES_ONE_COMMAND_INIT_FAILED] reason=protected Choir entities unavailable possibleVoice=%u manifestVoice=%u neme=%u researchLoom=%u"),
            SeveralVoicesPossibleVoiceId,
            SeveralVoicesManifestVoiceId,
            SeveralVoicesNemeId,
            SeveralVoicesResearchLoomId);
        Simulation.Reset();
        return false;
    }
    if (!SpawnTerrainView() || !SpawnFogView() || !SyncEntityViews(true))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_VIEW_INIT_FAILED] Initial visible entity views could not be created."));
        DestroyEntityViews();
        DestroyFogView();
        DestroyTerrainView();
        Simulation.Reset();
        bScenarioReady = false;
        bStressScenario = false;
        bSustainedStressScenario = false;
        return false;
    }
    bScenarioReady = true;
    if (bSustainedStressScenario &&
        !ValidateSustainedStressContract(true, false, false))
    {
        DestroyEntityViews();
        DestroyFogView();
        DestroyTerrainView();
        Simulation.Reset();
        return false;
    }

    if (bLumeReach)
    {
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            const FEchoesSeveralVoicesOneCommandPlan Plan =
                GetSeveralVoicesOneCommandPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_CHOIR_CRISIS_TERRAIN_READY] blocked=%d publicGates=3 possible=(%d,%d) manifest=(%d,%d) neme=(%d,%d) anchor=(%d,%d) inheritedBranch=%u planKey=%u"),
                GlassScarBlockedTiles,
                Plan.PossibleVoiceSite.x.FloorToInt(),
                Plan.PossibleVoiceSite.y.FloorToInt(),
                Plan.ManifestVoiceSite.x.FloorToInt(),
                Plan.ManifestVoiceSite.y.FloorToInt(),
                Plan.NemeCommandSite.x.FloorToInt(),
                Plan.NemeCommandSite.y.FloorToInt(),
                Plan.CrisisAnchorSite.x.FloorToInt(),
                Plan.CrisisAnchorSite.y.FloorToInt(),
                static_cast<uint8>(SevenAccountsBranch),
                Plan.StablePlanKey);
        }
        else if (SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            const FEchoesAssemblyOfTheMissingPlan Plan =
                GetAssemblyOfTheMissingPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_CROWNFALL_PUBLIC_INDEX_TERRAIN_READY] blocked=%d publicGates=3 index=(%d,%d) inheritedBranch=%u planKey=%u"),
                GlassScarBlockedTiles,
                Plan.CrownfallIndexSite.x.FloorToInt(),
                Plan.CrownfallIndexSite.y.FloorToInt(),
                static_cast<uint8>(SevenAccountsBranch),
                Plan.StablePlanKey);
        }
        else if (SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon)
        {
            const FEchoesFutureThatWonPlan Plan = GetFutureThatWonPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_LUME_RESTORATION_TERRAIN_READY] blocked=%d publicGates=3 demonstrator=(%d,%d) well=(%d,%d) inheritedBranch=%u planKey=%u"),
                GlassScarBlockedTiles,
                Plan.RestorationDemonstratorSite.x.FloorToInt(),
                Plan.RestorationDemonstratorSite.y.FloorToInt(),
                Plan.FutureWellSite.x.FloorToInt(),
                Plan.FutureWellSite.y.FloorToInt(),
                static_cast<uint8>(SevenAccountsBranch),
                Plan.StablePlanKey);
        }
        else if (SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            const FEchoesNoNeutralLedgerPlan Plan =
                GetNoNeutralLedgerPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_LUME_CONCORDANCE_TERRAIN_READY] blocked=%d publicGates=3 well=(%d,%d) inheritedBranch=%u planKey=%u"),
                GlassScarBlockedTiles,
                Plan.FutureWellSite.x.FloorToInt(),
                Plan.FutureWellSite.y.FloorToInt(),
                static_cast<uint8>(SevenAccountsBranch),
                Plan.StablePlanKey);
        }
        else
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_LUME_REACH_TERRAIN_READY] blocked=%d publicGates=3 well=(32,43) inheritedBranch=%u"),
                GlassScarBlockedTiles,
                static_cast<uint8>(SevenAccountsBranch));
        }
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_GLASS_SCAR_READY] blocked=%d crossings=3 centralWell=(32,32)"),
            GlassScarBlockedTiles);
    }

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_SIM_READY] Prototype initialized: %d entities, %d visible views, %u Hz, seed=%llu."),
        static_cast<int32>(Simulation->Entities().size()),
        EntityViews.Num(),
        Simulation->Config().ticksPerSecond,
        static_cast<unsigned long long>(Simulation->Config().randomSeed));
    if (!bStressScenario)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_FACTION_SCENARIO_READY] local=%s opposition=%s selectable=true"),
            FactionStableName(ScenarioLocalFaction),
            FactionStableName(ScenarioOpponentFaction));
        if (SelectedOperation == EEchoesOperationMode::CampaignPrologue)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_READY] mission=WhatTheLedgerKeeps carrier=%u archive=(22,18) evacuation=(6,17) faction=MeridianCompact completion=withdrawal"),
                ArchiveCarrierId);
        }
        else if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts)
        {
            const FEchoesSevenAccountsRoute Route = GetSevenAccountsRoute();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SEVEN_ACCOUNTS_READY] branch=%s waystone=%u bearer=%u anchor=(%d,%d) account=(%d,%d) terrainDelta=%d blocked=%d"),
                Route.StableName,
                MigrationWaystoneId,
                MemoryBearerId,
                Route.WaystoneAnchor.x.FloorToInt(),
                Route.WaystoneAnchor.y.FloorToInt(),
                Route.MemoryAccountSite.x.FloorToInt(),
                Route.MemoryAccountSite.y.FloorToInt(),
                SevenAccountsTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignCityReserve)
        {
            const FEchoesCityReserveGrid Grid = GetCityReserveGrid();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_CITY_RESERVE_READY] branch=%s priority=%s secondary=%s final=%s life=%u transit=%u archive=%u powered=0 inheritedRecords=2"),
                Grid.StableName,
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Priority),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Secondary),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Final),
                LifeSupportDistrictId,
                TransitDistrictId,
                ArchiveDistrictId);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignUnburiedRoad)
        {
            const FEchoesUnburiedRoadRoute Route = GetUnburiedRoadRoute();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_UNBURIED_ROAD_READY] branch=%s waystone=%u bearer=%u roadhead=(%d,%d) listeningSpine=(%d,%d) shard=(%d,%d) terrainDelta=%d blocked=%d inheritedRecords=3"),
                Route.StableName,
                MigrationWaystoneId,
                MemoryBearerId,
                Route.Roadhead.x.FloorToInt(),
                Route.Roadhead.y.FloorToInt(),
                Route.ListeningSpineSite.x.FloorToInt(),
                Route.ListeningSpineSite.y.FloorToInt(),
                Route.MemoryShardSite.x.FloorToInt(),
                Route.MemoryShardSite.y.FloorToInt(),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            const FEchoesTermsOfContinuancePlan Plan =
                GetTermsOfContinuancePlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_TERMS_OF_CONTINUANCE_READY] branch=%s meridianRelay=%u kharuunSpine=%u meridianWitness=%u kharuunWitness=%u relay=(%d,%d) spine=(%d,%d) extraction=(%d,%d) window=(%llu,%llu) pressureProxies=2 proxyAuthority=MeridianCompact pressureFaction=KharuunAssemblies pressureBehavior=genericAdaptive terrainDelta=%d blocked=%d inheritedRecords=4"),
                Plan.StableName,
                MeridianContinuanceRelayId,
                KharuunContinuanceSpineId,
                MeridianContinuanceWitnessId,
                KharuunContinuanceWitnessId,
                Plan.MeridianRelaySite.x.FloorToInt(),
                Plan.MeridianRelaySite.y.FloorToInt(),
                Plan.KharuunSpineSite.x.FloorToInt(),
                Plan.KharuunSpineSite.y.FloorToInt(),
                Plan.WitnessExtractionSite.x.FloorToInt(),
                Plan.WitnessExtractionSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowStartTick),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowEndTick),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            const FEchoesNamesWithoutBirthsPlan Plan =
                GetNamesWithoutBirthsPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_READY] branch=%s talar=%u archive=%u civilianA=%u civilianB=%u census=(%d,%d) shelter=(%d,%d) extraction=(%d,%d) pressureProxies=3 pressureFaction=KharuunAssemblies pressureBehavior=genericAdaptive inheritedRecords=5"),
                Plan.StableName,
                TalarId,
                CensusArchiveId,
                FirstCivilianId,
                SecondCivilianId,
                Plan.CensusSite.x.FloorToInt(),
                Plan.CensusSite.y.FloorToInt(),
                Plan.CivilianShelterSite.x.FloorToInt(),
                Plan.CivilianShelterSite.y.FloorToInt(),
                Plan.EvidenceExtractionSite.x.FloorToInt(),
                Plan.EvidenceExtractionSite.y.FloorToInt());
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignShapeOfSilence)
        {
            const FEchoesShapeOfSilencePlan Plan =
                GetShapeOfSilencePlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_OF_SILENCE_READY] branch=%s oruun=%u witnessA=%u witnessB=%u waystone=%u anchor=(%d,%d) spine=(%d,%d) witnessSites=(%d,%d):(%d,%d) confluence=(%d,%d) observedCorrespondenceOnly=true hiddenAttribution=false inheritedRecords=6 terrainDelta=%d blocked=%d"),
                Plan.StableName,
                OruunId,
                FirstMemoryWitnessId,
                SecondMemoryWitnessId,
                MigrationWaystoneId,
                Plan.WaystoneAnchor.x.FloorToInt(),
                Plan.WaystoneAnchor.y.FloorToInt(),
                Plan.ListeningSpineSite.x.FloorToInt(),
                Plan.ListeningSpineSite.y.FloorToInt(),
                Plan.FirstWitnessSite.x.FloorToInt(),
                Plan.FirstWitnessSite.y.FloorToInt(),
                Plan.SecondWitnessSite.x.FloorToInt(),
                Plan.SecondWitnessSite.y.FloorToInt(),
                Plan.ConfluenceSite.x.FloorToInt(),
                Plan.ConfluenceSite.y.FloorToInt(),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignShapeBesideUs)
        {
            const FEchoesShapeBesideUsPlan Plan = GetShapeBesideUsPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_BESIDE_US_READY] branch=%s talar=%u witnessA=%u witnessB=%u firstEcho=(%d,%d) relay=(%d,%d) stateSites=(%d,%d):(%d,%d) convergence=(%d,%d) reciprocalContactOnly=true hollowChoirCommandGranted=false hiddenAttribution=false inheritedRecords=7 terrainDelta=%d blocked=%d"),
                Plan.StableName,
                ShapeBesideUsTalarId,
                FirstStateWitnessId,
                SecondStateWitnessId,
                Plan.FirstEchoSite.x.FloorToInt(),
                Plan.FirstEchoSite.y.FloorToInt(),
                Plan.EchoRelaySite.x.FloorToInt(),
                Plan.EchoRelaySite.y.FloorToInt(),
                Plan.FirstStateSite.x.FloorToInt(),
                Plan.FirstStateSite.y.FloorToInt(),
                Plan.SecondStateSite.x.FloorToInt(),
                Plan.SecondStateSite.y.FloorToInt(),
                Plan.ConvergenceSite.x.FloorToInt(),
                Plan.ConvergenceSite.y.FloorToInt(),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignReserveAuthority)
        {
            const FEchoesReserveAuthorityPlan Plan =
                GetReserveAuthorityPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESERVE_AUTHORITY_READY] branch=%s mara=%u districts=%u,%u,%u authority=(%d,%d) recommended=%s allocation=exactlyTwo deferredMustSurvive=true localDecisionOnly=true widerCityRestored=false civilianSurvivalUnmodeled=true inheritedRecords=8 terrainDelta=%d blocked=%d"),
                Plan.StableName,
                ReserveAuthorityMaraId,
                LifeSupportDistrictId,
                TransitDistrictId,
                ArchiveDistrictId,
                Plan.AuthoritySite.x.FloorToInt(),
                Plan.AuthoritySite.y.FloorToInt(),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.RecommendedFirstDistrict),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignChoirAtLumeReach)
        {
            const FEchoesChoirAtLumeReachPlan Plan =
                GetChoirAtLumeReachPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_CHOIR_AT_LUME_REACH_READY] approach=%s priorBranch=%u deferredDistrict=%u oruun=%u waystone=%u well=%u contact=(%d,%d) liability=(%d,%d) anchors=(%d,%d):(%d,%d) wellSite=(%d,%d) localFaction=KharuunAssemblies maraPresence=liaisonOnly choirPresence=publicNonCommandableInMission10 opposition=meridianMechanicalQuarantineProxies maraInvolvementUnmodeled=true compactWideActionUnproven=true mixedFactionCommand=false hiddenAttribution=false inheritedRecords=9 blocked=%d"),
                Plan.StableName,
                static_cast<uint8>(Plan.PriorChoice),
                static_cast<uint8>(Plan.DeferredDistrict),
                ChoirAtLumeReachOruunId,
                ChoirAtLumeReachWaystoneId,
                ChoirAtLumeReachWellId,
                Plan.ContactSite.x.FloorToInt(),
                Plan.ContactSite.y.FloorToInt(),
                Plan.LiabilitySite.x.FloorToInt(),
                Plan.LiabilitySite.y.FloorToInt(),
                Plan.FirstAnchorSite.x.FloorToInt(),
                Plan.FirstAnchorSite.y.FloorToInt(),
                Plan.SecondAnchorSite.x.FloorToInt(),
                Plan.SecondAnchorSite.y.FloorToInt(),
                Plan.FutureWellSite.x.FloorToInt(),
                Plan.FutureWellSite.y.FloorToInt(),
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            const FEchoesNoNeutralLedgerPlan Plan =
                GetNoNeutralLedgerPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NO_NEUTRAL_LEDGER_READY] planKey=%u founding=%u route=%s districtA=%s districtB=%s deferred=%s lumeProtocol=%u protocol=%s oruun=%u waystone=%u witness=%u districtInterfaces=%u:%u evidenceInterfaces=%u:%u well=%u inheritedRecords=10 localFaction=KharuunAssemblies meridianPresence=neutralPoweredPublicInterfacesOnly choirPresence=publicNonCommandableInMission11 mixedFactionCommand=false hiddenTrust=false survivorVarianceUnmodeled=true proxyAttribution=false blocked=%d"),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                Plan.RouteStableName,
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.FirstContributingDistrict),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.SecondContributingDistrict),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.DeferredDistrict),
                static_cast<uint8>(Plan.LumeProtocol),
                Plan.ProtocolStableName,
                NoNeutralOruunId,
                NoNeutralWaystoneId,
                NoNeutralLedgerWitnessId,
                NoNeutralFirstDistrictInterfaceId,
                NoNeutralSecondDistrictInterfaceId,
                NoNeutralMeridianEvidenceInterfaceId,
                NoNeutralKharuunEvidenceInterfaceId,
                NoNeutralWellId,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignFutureThatWon)
        {
            const FEchoesFutureThatWonPlan Plan = GetFutureThatWonPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_FUTURE_THAT_WON_READY] planKey=%u founding=%u route=%s districtA=%s districtB=%s deferred=%s recordedProtocol=%u protocol=%s oruun=%u verifier=%u districtReadbacks=%u:%u evidenceReadbacks=%u:%u demonstrator=%u well=%u stabilityTicks=%llu inheritedRecords=11 localFaction=KharuunAssemblies rhysePresence=attributablePublicApparatusOnly meridianPresence=neutralPublicInterfacesOnly mixedFactionCommand=false civilianCountsUnmodeled=true populationRestorationUnproven=true permanentFutureUnproven=true blocked=%d"),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                Plan.RouteStableName,
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.FirstContributingDistrict),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.SecondContributingDistrict),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.DeferredDistrict),
                static_cast<uint8>(Plan.RecordedProtocol),
                Plan.ProtocolStableName,
                FutureWonOruunId,
                FutureWonVerifierId,
                FutureWonFirstDistrictInterfaceId,
                FutureWonSecondDistrictInterfaceId,
                FutureWonMeridianReadbackInterfaceId,
                FutureWonKharuunReadbackInterfaceId,
                FutureWonDemonstratorInterfaceId,
                FutureWonWellId,
                static_cast<unsigned long long>(Plan.StabilityWindowTicks),
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            const FEchoesAssemblyOfTheMissingPlan Plan =
                GetAssemblyOfTheMissingPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_ASSEMBLY_OF_THE_MISSING_READY] planKey=%u founding=%u route=%s districtA=%s districtB=%s deferred=%s recordedProtocol=%u protocol=%s oruun=%u verifier=%u publicRecords=%u:%u crownfallIndex=%u inheritedRecords=12 localFaction=KharuunAssemblies meridianPresence=neutralPublicRecordInterfaceOnly kharuunPublicRecord=neutralInterface crownfallIndex=neutralPublicInterface mixedFactionCommand=false responsibilityUnassigned=true hiddenAuthorshipUnproven=true trustUnproven=true consentUnproven=true civilianStateUnmodeled=true blocked=%d"),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                Plan.RouteStableName,
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.FirstContributingDistrict),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.SecondContributingDistrict),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.DeferredDistrict),
                static_cast<uint8>(Plan.RecordedProtocol),
                Plan.ProtocolStableName,
                AssemblyOruunId,
                AssemblyVerifierId,
                AssemblyMeridianPublicRecordInterfaceId,
                AssemblyKharuunPublicRecordInterfaceId,
                AssemblyCrownfallIndexInterfaceId,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            const FEchoesSeveralVoicesOneCommandPlan Plan =
                GetSeveralVoicesOneCommandPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SEVERAL_VOICES_ONE_COMMAND_READY] planKey=%u founding=%u route=%s recordedProtocol=%u protocol=%s possibleVoice=%u manifestVoice=%u neme=%u researchLoom=%u possibleSite=(%d,%d) manifestSite=(%d,%d) nemeSite=(%d,%d) anchorSite=(%d,%d) identityResolveTicks=160 crisisHoldTicks=%llu inheritedRecords=13 localFaction=HollowChoir localAuthority=HollowChoir incompatibleStates=true visibleTimers=true finalChoirFateDecided=false campaignBalanceUnproven=true blocked=%d"),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                Plan.RouteStableName,
                static_cast<uint8>(Plan.RecordedProtocol),
                Plan.ProtocolStableName,
                SeveralVoicesPossibleVoiceId,
                SeveralVoicesManifestVoiceId,
                SeveralVoicesNemeId,
                SeveralVoicesResearchLoomId,
                Plan.PossibleVoiceSite.x.FloorToInt(),
                Plan.PossibleVoiceSite.y.FloorToInt(),
                Plan.ManifestVoiceSite.x.FloorToInt(),
                Plan.ManifestVoiceSite.y.FloorToInt(),
                Plan.NemeCommandSite.x.FloorToInt(),
                Plan.NemeCommandSite.y.FloorToInt(),
                Plan.CrisisAnchorSite.x.FloorToInt(),
                Plan.CrisisAnchorSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    SeveralVoicesCrisisHoldTicks),
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignTheBrokenSun)
        {
            const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
            const echoes::sim::Vec2 RestorationSite =
                FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                    Plan,
                    EEchoesFinalResolution::Restoration);
            const echoes::sim::Vec2 ControlledSite =
                FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                    Plan,
                    EEchoesFinalResolution::ControlledStabilization);
            const echoes::sim::Vec2 ExtinguishmentSite =
                FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                    Plan,
                    EEchoesFinalResolution::Extinguishment);
            const echoes::sim::Vec2 EvolutionSite =
                FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                    Plan,
                    EEchoesFinalResolution::OpenEvolution);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_BROKEN_SUN_READY] planKey=%u founding=%u route=%s recordedProtocol=%u protocol=%s availability=0x%02X voice=%u heavy=%u neme=%u worker=%u mara=%u oruun=%u talar=%u approach=(%d,%d) maraSite=(%d,%d) oruunSite=(%d,%d) nemeSite=(%d,%d) talarSite=(%d,%d) restoration=(%d,%d) controlled=(%d,%d) extinguishment=(%d,%d) evolution=(%d,%d) baseHoldTicks=%llu inheritedRecords=14 localFaction=HollowChoir localAuthority=HollowChoir namedWitnesses=protectedNeutral mixedFactionCommand=false explicitEndingEligibility=true hiddenMoralityScore=false oldLedgerMigration=true oldCheckpointCompatibility=false broadConsequencesUnmodeled=true campaignBalanceUnproven=true ordinaryHumanCompletionUnproven=true releaseReadinessUnproven=true blocked=%d"),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                Plan.RouteStableName,
                static_cast<uint8>(Plan.RecordedProtocol),
                Plan.ProtocolStableName,
                Plan.AvailableFinalResolutions,
                BrokenSunAccordVoiceId,
                BrokenSunAccordHeavyId,
                BrokenSunNemeId,
                BrokenSunWorkerId,
                BrokenSunMaraId,
                BrokenSunOruunId,
                BrokenSunTalarId,
                Plan.CrownfallApproachSite.x.FloorToInt(),
                Plan.CrownfallApproachSite.y.FloorToInt(),
                Plan.MaraAccordSite.x.FloorToInt(),
                Plan.MaraAccordSite.y.FloorToInt(),
                Plan.OruunAccordSite.x.FloorToInt(),
                Plan.OruunAccordSite.y.FloorToInt(),
                Plan.NemeAccordSite.x.FloorToInt(),
                Plan.NemeAccordSite.y.FloorToInt(),
                Plan.TalarPublicRecordSite.x.FloorToInt(),
                Plan.TalarPublicRecordSite.y.FloorToInt(),
                RestorationSite.x.FloorToInt(),
                RestorationSite.y.FloorToInt(),
                ControlledSite.x.FloorToInt(),
                ControlledSite.y.FloorToInt(),
                ExtinguishmentSite.x.FloorToInt(),
                ExtinguishmentSite.y.FloorToInt(),
                EvolutionSite.x.FloorToInt(),
                EvolutionSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    Plan.ResolutionHoldTicks),
                GlassScarBlockedTiles);
        }
        const int32 PoweredAegisCount = static_cast<int32>(std::count_if(
            Simulation->Entities().begin(),
            Simulation->Entities().end(),
            [](const echoes::sim::Entity& Entity)
            {
                return Entity.faction ==
                           echoes::sim::Faction::MeridianCompact &&
                       Entity.type ==
                           echoes::sim::EntityType::UtilityStructure &&
                       Entity.aegisPowered;
            }));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_POWERED_AEGIS_READY] powered=%d publicState=true networkCounterplay=true"),
            PoweredAegisCount);
        if (bResearchPresentationScenario)
        {
            const echoes::sim::PlayerState* Player =
                Simulation->FindPlayer(LocalPlayerId);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESEARCH_PRESENTATION_READY] technology=%s producerQueued=true controlled=true release=false material=%d dawn=%d"),
                ResearchStableName(ResearchPresentationTechnology),
                Player != nullptr ? Player->resources.material : 0,
                Player != nullptr ? Player->resources.dawnshards : 0);
        }
        if (bResearchInterruptionPresentationScenario)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESEARCH_INTERRUPTION_PRESENTATION_READY] technology=%s attackers=32 attackTick=60 controlled=true release=false"),
                ResearchStableName(ResearchPresentationTechnology));
        }
        if (bKharuunSystemsPresentationScenario)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_READY] commands=3 hiddenMovers=1 controlled=true release=false"));
        }
        if (bPrologueCompletionPresentationScenario)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_READY] carrier=%u worker=%u well=%u protocol=Preserve controlled=true release=false ledgerPath=%s"),
                ArchiveCarrierId,
                ProloguePresentationWorkerId,
                ProloguePresentationWellId,
                *CampaignProgressPath);
        }
        if (bPointerCombatGuardPresentationScenario)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_POINTER_COMBAT_GUARD_PRESENTATION_READY] projectedLiveViews=true exactScreenCoordinates=true ordinaryControllerBindings=true controlledNonshipping=true"));
        }
    }
    if (bStressScenario && !bSustainedStressScenario)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_STRESS_ORDERS_READY] attackMove=%d teams=4 executeTick=1"),
            StressAttackMoveCommands);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_STRESS_READY] units=400 teams=4 entities=%d visibleViews=%d"),
            static_cast<int32>(Simulation->Entities().size()),
            EntityViews.Num());
    }
    else if (bSustainedStressScenario)
    {
        // Unreal can still perform renderer, font, and console-variable work
        // after BeginPlay returns. Keep the simulation at tick zero until a
        // bounded stable-frame window arms the sustained timing contract.
        SustainedStressReadyWallSeconds = 0.0;
    }
    return true;
}

void UEchoesSimulationSubsystem::StopPrototypeScenario()
{
    DestroyEntityViews();
    DestroyFogView();
    DestroyTerrainView();
    Simulation.Reset();
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bScenarioReady = false;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = false;
    bLoggedStressCombat = false;
    bLoggedAiExpansion = false;
    bLoggedAiRetreat = false;
    bLoggedAiPlayerView = false;
    bLoggedAiAdaptation = false;
    bLoggedAiMineralCover = false;
    bLoggedAiVibrationResponse = false;
    bResearchPresentationScenario = false;
    bResearchInterruptionPresentationScenario = false;
    bKharuunSystemsPresentationScenario = false;
    bPrologueCompletionPresentationScenario = false;
    bPointerCombatGuardPresentationScenario = false;
    bLoggedResearchPresentationActive = false;
    bLoggedResearchPresentationComplete = false;
    bLoggedResearchPresentationInterrupted = false;
    bLoggedKharuunSystemsPresentation = false;
    PrologueCompletionPresentationStage = 0;
    ProloguePresentationWorkerId = 0;
    ProloguePresentationWellId = 0;
    bSimulationPaused = false;
    bMatchResultReported = false;
    bStressScenario = false;
    bSustainedStressScenario = false;
    bSustainedStressFailed = false;
    bSustainedStressTimingReady = false;
    bSustainedStressQualificationLogged = false;
    SustainedStressFailureCode.Reset();
    SustainedStressCombatEntityIds.Reset();
    SustainedStressCombatOwners.Reset();
    SustainedStressCombatFactions.Reset();
    SustainedStressCombatTypes.Reset();
    SustainedStressCombatSpawnPositions.Reset();
    SustainedStressCommandCoreIds.fill(0);
    SustainedStressIntervalDamage = 0;
    SustainedStressIntervalCombatLosses = 0;
    SustainedStressCumulativeCombatLosses = 0;
    SustainedStressIntervalReplacements = 0;
    SustainedStressCumulativeReplacements = 0;
    SustainedStressIntervalOrderRenewals = 0;
    SustainedStressCumulativeOrderRenewals = 0;
    SustainedStressLastActivityTick = 0;
    SustainedStressLastHeartbeatTick = 0;
    SustainedStressLastHeartbeatWallMs = 0;
    SustainedStressStartupStableFrames = 0;
    SustainedStressStartupStableSeconds = 0.0;
    SustainedStressRenewalCursorByPlayer.fill(0);
    SustainedStressReadyWallSeconds = 0.0;
    ArchiveCarrierId = 0;
    MemoryBearerId = 0;
    MigrationWaystoneId = 0;
    LifeSupportDistrictId = 0;
    TransitDistrictId = 0;
    ArchiveDistrictId = 0;
    MeridianContinuanceRelayId = 0;
    KharuunContinuanceSpineId = 0;
    MeridianContinuanceWitnessId = 0;
    KharuunContinuanceWitnessId = 0;
    TalarId = 0;
    CensusArchiveId = 0;
    FirstCivilianId = 0;
    SecondCivilianId = 0;
    OruunId = 0;
    FirstMemoryWitnessId = 0;
    SecondMemoryWitnessId = 0;
    ShapeBesideUsTalarId = 0;
    FirstStateWitnessId = 0;
    SecondStateWitnessId = 0;
    ReserveAuthorityMaraId = 0;
    ChoirAtLumeReachOruunId = 0;
    ChoirAtLumeReachWaystoneId = 0;
    ChoirAtLumeReachWellId = 0;
    NoNeutralOruunId = 0;
    NoNeutralWaystoneId = 0;
    NoNeutralLedgerWitnessId = 0;
    NoNeutralFirstDistrictInterfaceId = 0;
    NoNeutralSecondDistrictInterfaceId = 0;
    NoNeutralMeridianEvidenceInterfaceId = 0;
    NoNeutralKharuunEvidenceInterfaceId = 0;
    NoNeutralWellId = 0;
    FutureWonOruunId = 0;
    FutureWonVerifierId = 0;
    FutureWonFirstDistrictInterfaceId = 0;
    FutureWonSecondDistrictInterfaceId = 0;
    FutureWonMeridianReadbackInterfaceId = 0;
    FutureWonKharuunReadbackInterfaceId = 0;
    FutureWonDemonstratorInterfaceId = 0;
    FutureWonWellId = 0;
    AssemblyOruunId = 0;
    AssemblyVerifierId = 0;
    AssemblyMeridianPublicRecordInterfaceId = 0;
    AssemblyKharuunPublicRecordInterfaceId = 0;
    AssemblyCrownfallIndexInterfaceId = 0;
    SeveralVoicesPossibleVoiceId = 0;
    SeveralVoicesManifestVoiceId = 0;
    SeveralVoicesNemeId = 0;
    SeveralVoicesResearchLoomId = 0;
    bSeveralVoicesCrisisHoldStarted = false;
    bSeveralVoicesCrisisContractFailed = false;
    BrokenSunAccordVoiceId = 0;
    BrokenSunAccordHeavyId = 0;
    BrokenSunNemeId = 0;
    BrokenSunWorkerId = 0;
    BrokenSunMaraId = 0;
    BrokenSunOruunId = 0;
    BrokenSunTalarId = 0;
    BrokenSunApproachAnchorId = 0;
    BrokenSunResolutionConduitId = 0;
    PendingBrokenSunResolution = EEchoesFinalResolution::None;
    SelectedBrokenSunResolution = EEchoesFinalResolution::None;
    bBrokenSunResolutionHoldStarted = false;
    bBrokenSunResolutionContractFailed = false;
    BrokenSunResolutionStartTick = 0;
    ResearchPresentationTechnology = echoes::sim::ResearchType::None;
}

bool UEchoesSimulationSubsystem::RestartPrototypeScenario()
{
    const bool bRestartStressScenario = bStressScenario;
    const bool bRestartSustainedStressScenario = bSustainedStressScenario;
    StopPrototypeScenario();
    const bool bRestarted = StartScenario(
        bRestartStressScenario,
        bRestartSustainedStressScenario);
    if (bRestarted)
    {
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_MATCH_RESTARTED]"));
    }
    else
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_MATCH_RESTART_FAILED]"));
    }
    return bRestarted;
}

bool UEchoesSimulationSubsystem::SelectLocalFaction(
    echoes::sim::Faction NewFaction,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (NewFaction != Faction::MeridianCompact &&
        NewFaction != Faction::KharuunAssemblies &&
        NewFaction != Faction::HollowChoir)
    {
        OutFeedback = TEXT("[FACTION_INVALID] That force is not playable in Glass Scar.");
        return false;
    }
    if (bStressScenario)
    {
        OutFeedback = TEXT("[FACTION_STRESS_LOCKED] The scale fixture has fixed teams.");
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_PROLOGUE_LOCKED] Mara Vey deploys with the Meridian Compact.");
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_SEVEN_ACCOUNTS_LOCKED] Oruun deploys with the Kharuun Assemblies.");
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_CITY_RESERVE_LOCKED] Mara Vey deploys with the Meridian Compact.");
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_UNBURIED_ROAD_LOCKED] Oruun deploys with the Kharuun Assemblies.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_TERMS_OF_CONTINUANCE_LOCKED] Meridian-authoritative treaty proxies are fixed for this prototype mission.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_NAMES_WITHOUT_BIRTHS_LOCKED] Talar's protected archive convoy deploys under Meridian command authority.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_SHAPE_OF_SILENCE_LOCKED] Oruun and both memory witnesses deploy under Kharuun authority.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_SHAPE_BESIDE_US_LOCKED] Talar and both state witnesses deploy under Meridian authority.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_RESERVE_AUTHORITY_LOCKED] Mara and the district reserve network deploy under Meridian authority.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_CHOIR_AT_LUME_REACH_LOCKED] Oruun and the Lume Reach listening force deploy under Kharuun authority; Mara remains an off-map liaison.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_NO_NEUTRAL_LEDGER_LOCKED] Oruun and the ledger witness deploy under Kharuun authority; Meridian and Choir contributions remain public interfaces, not commandable forces.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_FUTURE_THAT_WON_LOCKED] Oruun and the independent verifier deploy under Kharuun authority; Rhyse's demonstrator and Meridian readbacks remain public, non-commandable interfaces.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_ASSEMBLY_OF_THE_MISSING_LOCKED] Oruun and the independent verifier deploy under Kharuun authority; all three public record interfaces remain neutral and non-commandable.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        NewFaction != Faction::HollowChoir)
    {
        OutFeedback = TEXT("[FACTION_SEVERAL_VOICES_ONE_COMMAND_LOCKED] Neme and the protected voices deploy under Hollow Choir authority.");
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun &&
        NewFaction != Faction::HollowChoir)
    {
        OutFeedback = TEXT("[FACTION_THE_BROKEN_SUN_LOCKED] The final operation retains Hollow Choir command; Mara, Oruun, and Talar are protected neutral witnesses.");
        return false;
    }
    if (NewFaction == LocalFaction)
    {
        OutFeedback = FString::Printf(
            TEXT("FACTION: %s already selected."),
            FactionStableName(LocalFaction));
        return true;
    }

    const Faction PreviousFaction = LocalFaction;
    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    if (!bHadScenario)
    {
        LocalFaction = NewFaction;
        OutFeedback = FString::Printf(
            TEXT("FACTION SELECTED: %s."),
            FactionStableName(LocalFaction));
        return true;
    }

    StopPrototypeScenario();
    LocalFaction = NewFaction;
    if (StartScenario(false))
    {
        SetScenarioPaused(bWasPaused);
        OutFeedback = FString::Printf(
            TEXT("FACTION SELECTED: %s. Glass Scar reset for deployment."),
            FactionStableName(LocalFaction));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_FACTION_SELECTED] local=%s scenarioReset=true paused=%s"),
            FactionStableName(LocalFaction),
            bWasPaused ? TEXT("true") : TEXT("false"));
        return true;
    }

    StopPrototypeScenario();
    LocalFaction = PreviousFaction;
    const bool bRollbackSucceeded = StartScenario(false);
    if (bRollbackSucceeded)
    {
        SetScenarioPaused(bWasPaused);
    }
    OutFeedback = bRollbackSucceeded
                      ? TEXT("[FACTION_REBUILD_FAILED] The prior faction was restored.")
                      : TEXT("[FACTION_ROLLBACK_FAILED] The operation could not be restored.");
    return false;
}

bool UEchoesSimulationSubsystem::SelectOperationMode(
    EEchoesOperationMode NewOperation,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (bStressScenario)
    {
        OutFeedback = TEXT("[OPERATION_STRESS_LOCKED] The scale fixture has a fixed operation.");
        return false;
    }
    if (NewOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        !IsSevenAccountsUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete What the Ledger Keeps before Seven Accounts of Rain.");
        return false;
    }
    if (NewOperation == EEchoesOperationMode::CampaignCityReserve &&
        !IsCityReserveUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Seven Accounts of Rain with a consistent ledger before A City on Reserve.");
        return false;
    }
    if (NewOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        !IsUnburiedRoadUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete A City on Reserve with a consistent ledger before The Unburied Road.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        !IsTermsOfContinuanceUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete The Unburied Road with a consistent ledger before Terms of Continuance.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        !IsNamesWithoutBirthsUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Terms of Continuance with a consistent ledger before Names Without Births.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        !IsShapeOfSilenceUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Names Without Births with a consistent ledger before The Shape of Silence.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs &&
        !IsShapeBesideUsUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete The Shape of Silence with a consistent ledger before The Shape Beside Us.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignReserveAuthority &&
        !IsReserveAuthorityUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete The Shape Beside Us with a consistent ledger before Reserve Authority.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach &&
        !IsChoirAtLumeReachUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Reserve Authority with a consistent ledger before The Choir at Lume Reach.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger &&
        !IsNoNeutralLedgerUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete The Choir at Lume Reach with the required ordered M01-M10 ledger prefix before No Neutral Ledger.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignFutureThatWon &&
        !IsFutureThatWonUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete No Neutral Ledger with the required ordered M01-M11 ledger prefix before The Future That Won.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
        !IsAssemblyOfTheMissingUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete The Future That Won with the required ordered M01-M12 ledger prefix before Assembly of the Missing.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        !IsSeveralVoicesOneCommandUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Assembly of the Missing with the required ordered M01-M13 ledger prefix before Several Voices, One Command.");
        return false;
    }
    if (NewOperation == EEchoesOperationMode::CampaignTheBrokenSun &&
        !IsBrokenSunUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Several Voices, One Command with the exact fourteen-record ledger before The Broken Sun.");
        return false;
    }
    if (NewOperation == SelectedOperation)
    {
        OutFeedback = FString::Printf(TEXT("OPERATION: %s already selected."), *GetOperationLabel());
        return true;
    }

    const EEchoesOperationMode PreviousOperation = SelectedOperation;
    const Faction PreviousFaction = LocalFaction;
    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    if (bHadScenario)
    {
        StopPrototypeScenario();
    }
    SelectedOperation = NewOperation;
    (void)TryGetFixedCampaignFaction(SelectedOperation, LocalFaction);
    if (!bHadScenario || StartScenario(false))
    {
        if (bHadScenario)
        {
            SetScenarioPaused(bWasPaused);
        }
        OutFeedback = FString::Printf(
            TEXT("OPERATION SELECTED: %s%s"),
            *GetOperationLabel(),
            SelectedOperation == EEchoesOperationMode::CampaignPrologue
                ? TEXT(" — Mara Vey's Meridian force is locked for this mission.")
            : SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT(" — Oruun's Kharuun migration force is locked for this mission.")
            : SelectedOperation == EEchoesOperationMode::CampaignCityReserve
                ? TEXT(" — Mara Vey's Meridian grid force is locked for this mission.")
            : SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT(" — Oruun's Kharuun road force is locked for this mission.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT(" — Meridian-authoritative treaty proxies are locked for this prototype mission.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT(" — Talar and the civilian archive convoy are locked under Meridian authority.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignShapeOfSilence
                ? TEXT(" — Oruun and both memory witnesses are locked under Kharuun authority.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignShapeBesideUs
                ? TEXT(" — Talar and both state witnesses are locked under Meridian authority.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignReserveAuthority
                ? TEXT(" — Mara and the district reserve network are locked under Meridian authority.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignChoirAtLumeReach
                ? TEXT(" — Oruun's Kharuun listening force is locked; Mara remains an off-map liaison and the local Choir is not commandable.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignNoNeutralLedger
                ? TEXT(" — Oruun's Kharuun force is locked; Meridian evidence and local Choir contact remain public, non-commandable interfaces.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignFutureThatWon
                ? TEXT(" — Oruun and the independent verifier are locked under Kharuun authority; Rhyse's apparatus remains public and non-commandable.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignAssemblyOfTheMissing
                ? TEXT(" — Oruun and the independent verifier are locked under Kharuun authority; the Meridian, Kharuun, and Crownfall public record interfaces remain neutral and non-commandable.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignSeveralVoicesOneCommand
                ? TEXT(" — Neme and the local Hollow Choir are commandable; incompatible identity states must resolve on visible timers.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignTheBrokenSun
                ? TEXT(" — the local Hollow Choir remains commandable; Mara, Oruun, and Talar are protected neutral witnesses to an explicit earned ending choice.")
                : TEXT("."));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_OPERATION_SELECTED] operation=%s scenarioReset=%s paused=%s"),
            SelectedOperation == EEchoesOperationMode::CampaignPrologue
                ? TEXT("WhatTheLedgerKeeps")
            : SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT("SevenAccountsOfRain")
            : SelectedOperation == EEchoesOperationMode::CampaignCityReserve
                ? TEXT("ACityOnReserve")
            : SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT("TheUnburiedRoad")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("TermsOfContinuance")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT("NamesWithoutBirths")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignShapeOfSilence
                ? TEXT("TheShapeOfSilence")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignShapeBesideUs
                ? TEXT("TheShapeBesideUs")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignReserveAuthority
                ? TEXT("ReserveAuthority")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignChoirAtLumeReach
                ? TEXT("ChoirAtLumeReach")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignNoNeutralLedger
                ? TEXT("NoNeutralLedger")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignFutureThatWon
                ? TEXT("TheFutureThatWon")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignAssemblyOfTheMissing
                ? TEXT("AssemblyOfTheMissing")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignSeveralVoicesOneCommand
                ? TEXT("SeveralVoicesOneCommand")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignTheBrokenSun
                ? TEXT("TheBrokenSun")
                : TEXT("GlassScar"),
            bHadScenario ? TEXT("true") : TEXT("false"),
            bWasPaused ? TEXT("true") : TEXT("false"));
        return true;
    }

    StopPrototypeScenario();
    SelectedOperation = PreviousOperation;
    LocalFaction = PreviousFaction;
    const bool bRollbackSucceeded = !bHadScenario || StartScenario(false);
    if (bRollbackSucceeded && bHadScenario)
    {
        SetScenarioPaused(bWasPaused);
    }
    OutFeedback = bRollbackSucceeded
                      ? TEXT("[OPERATION_REBUILD_FAILED] The prior operation was restored.")
                      : TEXT("[OPERATION_ROLLBACK_FAILED] The operation could not be restored.");
    return false;
}

bool UEchoesSimulationSubsystem::StartNewCampaign(FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!bCampaignProgressAvailable)
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] A new campaign could not be created because campaign storage is unavailable.");
        return false;
    }
    if (CampaignProgress.Decisions.IsEmpty())
    {
        OutFeedback = TEXT("NEW CAMPAIGN: the campaign ledger is already empty.");
        return true;
    }

    const FEchoesCampaignProgress PreviousCampaign = CampaignProgress;
    const EEchoesOperationMode PreviousOperation = SelectedOperation;
    const Faction PreviousFaction = LocalFaction;
    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    const int32 ReplacedDecisionCount = CampaignProgress.Decisions.Num();
    FEchoesCampaignProgress EmptyCampaign;
    CampaignProgress = MoveTemp(EmptyCampaign);
    RefreshCampaignBackupState();
    if (bHadScenario)
    {
        StopPrototypeScenario();
    }
    const FEchoesCampaignJourney NewJourney = GetCampaignJourney();
    SelectedOperation =
        NewJourney.State == EEchoesCampaignJourneyState::Ready
            ? NewJourney.NextOperation
            : EEchoesOperationMode::Skirmish;
    LocalFaction = Faction::MeridianCompact;
    (void)TryGetFixedCampaignFaction(SelectedOperation, LocalFaction);
    if (bHadScenario && !StartScenario(false))
    {
        StopPrototypeScenario();
        CampaignProgress = PreviousCampaign;
        RefreshCampaignBackupState();
        SelectedOperation = PreviousOperation;
        LocalFaction = PreviousFaction;
        const bool bScenarioRollbackSucceeded =
            StartScenario(false);
        if (bScenarioRollbackSucceeded)
        {
            SetScenarioPaused(bWasPaused);
        }
        OutFeedback = bScenarioRollbackSucceeded
            ? TEXT("[NEW_CAMPAIGN_REBUILD_FAILED] Mission 01 could not be rebuilt; the prior campaign and title scenario were restored.")
            : TEXT("[NEW_CAMPAIGN_ROLLBACK_FAILED] Mission 01 could not be rebuilt and the prior title scenario could not be restored. The durable campaign ledger was not changed.");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NEW_CAMPAIGN_FAILED] stage=scenario_rebuild replacedRecords=%d durableLedgerUntouched=true scenarioRollback=%s"),
            ReplacedDecisionCount,
            bScenarioRollbackSucceeded ? TEXT("true") : TEXT("false"));
        return false;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            CampaignProgress,
            SaveFeedback))
    {
        if (bHadScenario)
        {
            StopPrototypeScenario();
        }
        CampaignProgress = PreviousCampaign;
        RefreshCampaignBackupState();
        SelectedOperation = PreviousOperation;
        LocalFaction = PreviousFaction;
        const bool bScenarioRollbackSucceeded =
            !bHadScenario || StartScenario(false);
        if (bScenarioRollbackSucceeded && bHadScenario)
        {
            SetScenarioPaused(bWasPaused);
        }
        OutFeedback = bScenarioRollbackSucceeded
            ? FString::Printf(
                  TEXT("[NEW_CAMPAIGN_SAVE_FAILED] Mission 01 was rebuilt, but the empty ledger could not be committed; the prior campaign and title scenario were restored. %s"),
                  *SaveFeedback)
            : FString::Printf(
                  TEXT("[NEW_CAMPAIGN_ROLLBACK_FAILED] The empty ledger could not be committed and the prior title scenario could not be restored. %s"),
                  *SaveFeedback);
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NEW_CAMPAIGN_FAILED] stage=storage_commit replacedRecords=%d scenarioRollback=%s detail=%s"),
            ReplacedDecisionCount,
            bScenarioRollbackSucceeded ? TEXT("true") : TEXT("false"),
            *SaveFeedback);
        return false;
    }
    RefreshCampaignBackupState();
    if (bHadScenario)
    {
        SetScenarioPaused(bWasPaused);
    }
    OutFeedback = FString::Printf(
        TEXT("NEW CAMPAIGN CREATED: %d prior mission record%s replaced; one prior ledger generation retained as backup. Mission 01 is ready."),
        ReplacedDecisionCount,
        ReplacedDecisionCount == 1 ? TEXT("") : TEXT("s"));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NEW_CAMPAIGN_CREATED] replacedRecords=%d operation=WhatTheLedgerKeeps local=MeridianCompact backupRetained=true scenarioReset=%s"),
        ReplacedDecisionCount,
        bHadScenario ? TEXT("true") : TEXT("false"));
    return true;
}

void UEchoesSimulationSubsystem::RefreshCampaignBackupState()
{
    CampaignBackupProgress = FEchoesCampaignProgress{};
    bCampaignBackupAvailable = false;
    if (!bCampaignProgressAvailable)
    {
        return;
    }

    FString BackupFeedback;
    FEchoesCampaignProgress Candidate;
    if (!FEchoesCampaignProgressStore::LoadGeneration(
            CampaignProgressPath + TEXT(".bak"),
            Candidate,
            BackupFeedback))
    {
        return;
    }
    if (Candidate.Decisions == CampaignProgress.Decisions)
    {
        return;
    }
    CampaignBackupProgress = MoveTemp(Candidate);
    bCampaignBackupAvailable = true;
}

bool UEchoesSimulationSubsystem::RestoreCampaignBackup(FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!bCampaignProgressAvailable)
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Campaign recovery is unavailable because campaign storage is unavailable.");
        return false;
    }

    RefreshCampaignBackupState();
    if (!bCampaignBackupAvailable)
    {
        OutFeedback = TEXT("[CAMPAIGN_BACKUP_UNAVAILABLE] No distinct validated prior campaign generation is available.");
        return false;
    }

    const FEchoesCampaignProgress PreviousCampaign = CampaignProgress;
    const FEchoesCampaignProgress RestoredCampaign = CampaignBackupProgress;
    const EEchoesOperationMode PreviousOperation = SelectedOperation;
    const Faction PreviousFaction = LocalFaction;
    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    const int32 ReplacedDecisionCount = CampaignProgress.Decisions.Num();
    const int32 RestoredDecisionCount = RestoredCampaign.Decisions.Num();
    CampaignProgress = RestoredCampaign;
    RefreshCampaignBackupState();
    if (bHadScenario)
    {
        StopPrototypeScenario();
    }
    const FEchoesCampaignJourney RestoredJourney = GetCampaignJourney();
    SelectedOperation =
        RestoredJourney.State == EEchoesCampaignJourneyState::Ready
            ? RestoredJourney.NextOperation
            : EEchoesOperationMode::Skirmish;
    LocalFaction = Faction::MeridianCompact;
    (void)TryGetFixedCampaignFaction(SelectedOperation, LocalFaction);
    if (bHadScenario && !StartScenario(false))
    {
        StopPrototypeScenario();
        CampaignProgress = PreviousCampaign;
        RefreshCampaignBackupState();
        SelectedOperation = PreviousOperation;
        LocalFaction = PreviousFaction;
        const bool bScenarioRollbackSucceeded =
            StartScenario(false);
        if (bScenarioRollbackSucceeded)
        {
            SetScenarioPaused(bWasPaused);
        }
        OutFeedback = bScenarioRollbackSucceeded
            ? TEXT("[CAMPAIGN_RESTORE_REBUILD_FAILED] The restored campaign could not be rebuilt; the prior campaign and title scenario were restored.")
            : TEXT("[CAMPAIGN_RESTORE_ROLLBACK_FAILED] The restored campaign could not be rebuilt and the prior title scenario could not be restored. The durable campaign generations were not changed.");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CAMPAIGN_RESTORE_FAILED] stage=scenario_rebuild restoredRecords=%d replacedRecords=%d durableGenerationsUntouched=true scenarioRollback=%s"),
            RestoredDecisionCount,
            ReplacedDecisionCount,
            bScenarioRollbackSucceeded ? TEXT("true") : TEXT("false"));
        return false;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            CampaignProgress,
            SaveFeedback))
    {
        if (bHadScenario)
        {
            StopPrototypeScenario();
        }
        CampaignProgress = PreviousCampaign;
        RefreshCampaignBackupState();
        SelectedOperation = PreviousOperation;
        LocalFaction = PreviousFaction;
        const bool bScenarioRollbackSucceeded =
            !bHadScenario || StartScenario(false);
        if (bScenarioRollbackSucceeded && bHadScenario)
        {
            SetScenarioPaused(bWasPaused);
        }
        OutFeedback = bScenarioRollbackSucceeded
            ? FString::Printf(
                  TEXT("[CAMPAIGN_RESTORE_SAVE_FAILED] The restored mission was rebuilt, but its ledger could not be committed; the prior campaign and title scenario were restored. %s"),
                  *SaveFeedback)
            : FString::Printf(
                  TEXT("[CAMPAIGN_RESTORE_ROLLBACK_FAILED] The restored ledger could not be committed and the prior title scenario could not be restored. %s"),
                  *SaveFeedback);
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CAMPAIGN_RESTORE_FAILED] stage=storage_commit restoredRecords=%d replacedRecords=%d scenarioRollback=%s detail=%s"),
            RestoredDecisionCount,
            ReplacedDecisionCount,
            bScenarioRollbackSucceeded ? TEXT("true") : TEXT("false"),
            *SaveFeedback);
        return false;
    }
    RefreshCampaignBackupState();
    if (bHadScenario)
    {
        SetScenarioPaused(bWasPaused);
    }
    if (RestoredJourney.State == EEchoesCampaignJourneyState::Ready)
    {
        OutFeedback = FString::Printf(
            TEXT("CAMPAIGN RESTORED: prior generation with %d mission record%s is active; the replaced %d-record generation is retained as backup. %s is ready."),
            RestoredDecisionCount,
            RestoredDecisionCount == 1 ? TEXT("") : TEXT("s"),
            ReplacedDecisionCount,
            FEchoesCampaignJourneyModel::OperationDisplayName(
                RestoredJourney.NextOperation));
    }
    else if (RestoredJourney.State == EEchoesCampaignJourneyState::Complete)
    {
        OutFeedback = FString::Printf(
            TEXT("CAMPAIGN RESTORED: the completed fifteen-record campaign is active; the replaced %d-record generation is retained as backup."),
            ReplacedDecisionCount);
    }
    else
    {
        OutFeedback = FString::Printf(
            TEXT("CAMPAIGN RESTORED: prior generation with %d mission record%s is active; the replaced %d-record generation is retained as backup, but its campaign continuation is unavailable."),
            RestoredDecisionCount,
            RestoredDecisionCount == 1 ? TEXT("") : TEXT("s"),
            ReplacedDecisionCount);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESTORED] restoredRecords=%d replacedRecords=%d operation=%s local=%u journeyState=%u activeRetainedAsBackup=true scenarioReset=%s"),
        RestoredDecisionCount,
        ReplacedDecisionCount,
        FEchoesCampaignJourneyModel::OperationDisplayName(SelectedOperation),
        static_cast<uint8>(LocalFaction),
        static_cast<uint8>(RestoredJourney.State),
        bHadScenario ? TEXT("true") : TEXT("false"));
    return true;
}

FString UEchoesSimulationSubsystem::GetQuickSavePath()
{
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("EchoesQuickSave.bin"));
}

FString UEchoesSimulationSubsystem::GetActiveQuickSavePath() const
{
#if !UE_BUILD_SHIPPING
    FString QuickSavePathOverride;
    if (FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesQuickSavePath="),
            QuickSavePathOverride) &&
        !QuickSavePathOverride.IsEmpty())
    {
        return FPaths::ConvertRelativePathToFull(QuickSavePathOverride);
    }
#endif
    if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun)
    {
        FEchoesCampaignProgress PrerequisiteLedger;
        FEchoesBrokenSunPlan Plan;
        TArray<uint8> LedgerBytes;
        FString ProjectionError;
        if (BuildBrokenSunPrerequisiteProjection(
                CampaignProgress,
                PrerequisiteLedger,
                Plan,
                LedgerBytes,
                ProjectionError))
        {
            const uint32 LedgerFingerprint = FCrc::MemCrc32(
                LedgerBytes.GetData(),
                LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
            return FPaths::Combine(
                FEchoesCampaignProgressStore::GetSaveGameDirectory(),
                FString::Printf(
                    TEXT("EchoesQuickSaveTheBrokenSun-%08X.bin"),
                    LedgerFingerprint));
        }
        return FPaths::Combine(
            FEchoesCampaignProgressStore::GetSaveGameDirectory(),
            TEXT("EchoesQuickSaveTheBrokenSun-InvalidLedger.bin"));
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignPrologue)
    {
        return FPaths::Combine(
            FEchoesCampaignProgressStore::GetSaveGameDirectory(),
            TEXT("EchoesQuickSaveWhatTheLedgerKeeps.bin"));
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts)
    {
        return FPaths::Combine(
            FEchoesCampaignProgressStore::GetSaveGameDirectory(),
            TEXT("EchoesQuickSaveSevenAccountsOfRain.bin"));
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve)
    {
        return FPaths::Combine(
            FEchoesCampaignProgressStore::GetSaveGameDirectory(),
            TEXT("EchoesQuickSaveACityOnReserve.bin"));
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad)
    {
        return FPaths::Combine(
            FEchoesCampaignProgressStore::GetSaveGameDirectory(),
            TEXT("EchoesQuickSaveTheUnburiedRoad.bin"));
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
    {
        TArray<uint8> LedgerBytes;
        FString LedgerError;
        if (FEchoesCampaignProgressStore::Encode(
                CampaignProgress,
                LedgerBytes,
                LedgerError) &&
            !LedgerBytes.IsEmpty())
        {
            const uint32 LedgerFingerprint =
                FCrc::MemCrc32(
                    LedgerBytes.GetData(),
                    LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
            const TCHAR* Prefix =
                SelectedOperation ==
                        EEchoesOperationMode::CampaignTermsOfContinuance
                    ? TEXT("EchoesQuickSaveTermsOfContinuance")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignNamesWithoutBirths
                    ? TEXT("EchoesQuickSaveNamesWithoutBirths")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignShapeOfSilence
                    ? TEXT("EchoesQuickSaveTheShapeOfSilence")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignShapeBesideUs
                    ? TEXT("EchoesQuickSaveTheShapeBesideUs")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignReserveAuthority
                    ? TEXT("EchoesQuickSaveReserveAuthority")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignChoirAtLumeReach
                    ? TEXT("EchoesQuickSaveTheChoirAtLumeReach")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignNoNeutralLedger
                    ? TEXT("EchoesQuickSaveNoNeutralLedger")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignFutureThatWon
                    ? TEXT("EchoesQuickSaveTheFutureThatWon")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignAssemblyOfTheMissing
                    ? TEXT("EchoesQuickSaveAssemblyOfTheMissing")
                    : TEXT("EchoesQuickSaveSeveralVoicesOneCommand");
            return FPaths::Combine(
                FEchoesCampaignProgressStore::GetSaveGameDirectory(),
                FString::Printf(TEXT("%s-%08X.bin"), Prefix, LedgerFingerprint));
        }
        return FPaths::Combine(
            FEchoesCampaignProgressStore::GetSaveGameDirectory(),
            SelectedOperation ==
                    EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("EchoesQuickSaveTermsOfContinuance-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT("EchoesQuickSaveNamesWithoutBirths-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignShapeOfSilence
                ? TEXT("EchoesQuickSaveTheShapeOfSilence-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignShapeBesideUs
                ? TEXT("EchoesQuickSaveTheShapeBesideUs-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignReserveAuthority
                ? TEXT("EchoesQuickSaveReserveAuthority-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignChoirAtLumeReach
                ? TEXT("EchoesQuickSaveTheChoirAtLumeReach-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignNoNeutralLedger
                ? TEXT("EchoesQuickSaveNoNeutralLedger-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignFutureThatWon
                ? TEXT("EchoesQuickSaveTheFutureThatWon-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignAssemblyOfTheMissing
                ? TEXT("EchoesQuickSaveAssemblyOfTheMissing-InvalidLedger.bin")
                : TEXT("EchoesQuickSaveSeveralVoicesOneCommand-InvalidLedger.bin"));
    }
    return GetQuickSavePath();
}

EEchoesCampaignCommitStatus UEchoesSimulationSubsystem::CommitPrologueCompletion(
    echoes::sim::FutureWellChoice CurrentChoice,
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    OutRecordedChoice = CurrentChoice;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignPrologue ||
        GetProloguePhase() != EEchoesProloguePhase::Complete ||
        CurrentChoice == echoes::sim::FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative prologue can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    Record.WellChoice = CurrentChoice;
    Record.AvailableWellChoices = 0x07;
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitSevenAccountsCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignSevenAccounts ||
        GetSevenAccountsPhase() != EEchoesSevenAccountsPhase::Complete ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Seven Accounts operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::SevenAccountsOfRain;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::WaystoneRootedAtAnchor) |
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::MemoryBearerArrived) |
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::PriorDecisionConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitCityReserveCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignCityReserve ||
        GetCityReservePhase() != EEchoesCityReservePhase::Complete ||
        !IsCityReserveUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative City on Reserve operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::ACityOnReserve;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesCityReserveCompletionFact::LifeSupportPowered) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::TransitPowered) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::ArchivePowered) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitUnburiedRoadCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignUnburiedRoad ||
        GetUnburiedRoadPhase() != EEchoesUnburiedRoadPhase::Complete ||
        !IsUnburiedRoadUnlocked() || Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Unburied Road operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheUnburiedRoad;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::WaystoneRootedAtRoadhead) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::ListeningSpineRaised) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::MemoryShardRecovered) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitTermsOfContinuanceCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignTermsOfContinuance ||
        GetTermsOfContinuancePhase() !=
            EEchoesTermsOfContinuancePhase::Complete ||
        !IsTermsOfContinuanceUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Terms of Continuance operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TermsOfContinuance;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::MeridianRelaySynchronized) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::KharuunSpineSynchronized) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::ContinuanceWindowHeld) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::BothWitnessesExtracted) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitNamesWithoutBirthsCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignNamesWithoutBirths ||
        GetNamesWithoutBirthsPhase() !=
            EEchoesNamesWithoutBirthsPhase::Complete ||
        !IsNamesWithoutBirthsUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Names Without Births operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::NamesWithoutBirths;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::CensusEvidenceLocated) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::ArchivePowered) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::BothCiviliansSheltered) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::EvidenceExtracted) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitShapeOfSilenceCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignShapeOfSilence ||
        GetShapeOfSilencePhase() !=
            EEchoesShapeOfSilencePhase::Complete ||
        !IsShapeOfSilenceUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative The Shape of Silence operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheShapeOfSilence;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::WaystoneRootedAtListeningAnchor) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::ListeningSpineRaised) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::BothMemoryWitnessesPositioned) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::OruunReachedConfluence) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitShapeBesideUsCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignShapeBesideUs ||
        GetShapeBesideUsPhase() !=
            EEchoesShapeBesideUsPhase::Complete ||
        !IsShapeBesideUsUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative The Shape Beside Us operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheShapeBesideUs;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::FirstEchoObserved) |
        static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::EchoRelayRaised) |
        static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::BothStatesTraversed) |
        static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::NemeConvergenceReached) |
        static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesShapeBesideUsCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitReserveAuthorityCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    EEchoesCityDistrict& OutRecordedDeferredDistrict,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    OutRecordedDeferredDistrict = GetReserveAuthorityDeferredDistrict();
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignReserveAuthority ||
        GetReserveAuthorityPhase() !=
            EEchoesReserveAuthorityPhase::Complete ||
        !IsReserveAuthorityUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Reserve Authority operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::ReserveAuthority;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::ReserveAuthoritySecured) |
        static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::DeferredDistrictReached) |
        static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesReserveAuthorityCompletionFact::PriorLedgerConsumed);
    const echoes::sim::Entity* LifeSupport =
        Simulation->FindEntity(LifeSupportDistrictId);
    const echoes::sim::Entity* Transit =
        Simulation->FindEntity(TransitDistrictId);
    const echoes::sim::Entity* Archive =
        Simulation->FindEntity(ArchiveDistrictId);
    if (LifeSupport != nullptr && LifeSupport->aegisPowered)
    {
        Record.VerifiedFacts |= static_cast<uint8>(
            EEchoesReserveAuthorityCompletionFact::LifeSupportPowered);
    }
    if (Transit != nullptr && Transit->aegisPowered)
    {
        Record.VerifiedFacts |= static_cast<uint8>(
            EEchoesReserveAuthorityCompletionFact::TransitPowered);
    }
    if (Archive != nullptr && Archive->aegisPowered)
    {
        Record.VerifiedFacts |= static_cast<uint8>(
            EEchoesReserveAuthorityCompletionFact::ArchivePowered);
    }
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
        const uint8 Facts = Existing->VerifiedFacts;
        OutRecordedDeferredDistrict =
            (Facts & static_cast<uint8>(
                 EEchoesReserveAuthorityCompletionFact::LifeSupportPowered)) == 0
                ? EEchoesCityDistrict::LifeSupport
            : (Facts & static_cast<uint8>(
                   EEchoesReserveAuthorityCompletionFact::TransitPowered)) == 0
                ? EEchoesCityDistrict::Transit
                : EEchoesCityDistrict::Archive;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitChoirAtLumeReachCompletion(
    echoes::sim::FutureWellChoice CurrentChoice,
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    OutRecordedChoice = CurrentChoice;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignChoirAtLumeReach ||
        GetChoirAtLumeReachPhase() !=
            EEchoesChoirAtLumeReachPhase::Complete ||
        !IsChoirAtLumeReachUnlocked() ||
        CurrentChoice == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative The Choir at Lume Reach operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::ChoirAtLumeReach;
    Record.WellChoice = CurrentChoice;
    Record.AvailableWellChoices = 0x07;
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::ContactEstablished) |
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::DeferredLiabilityResolved) |
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::BothAnchorsRaised) |
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::WellChoiceCommitted) |
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::BranchResolutionCompleted) |
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::OruunSurvived) |
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesChoirAtLumeReachCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitNoNeutralLedgerCompletion(
    echoes::sim::FutureWellChoice& OutRecordedProtocol,
    FString& OutFeedback)
{
    const FEchoesNoNeutralLedgerPlan Plan = GetNoNeutralLedgerPlan();
    OutRecordedProtocol = Plan.LumeProtocol;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignNoNeutralLedger ||
        GetNoNeutralLedgerPhase() !=
            EEchoesNoNeutralLedgerPhase::Complete ||
        !IsNoNeutralLedgerUnlocked() ||
        Plan.LumeProtocol == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative No Neutral Ledger operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::NoNeutralLedger;
    Record.WellChoice = Plan.LumeProtocol;
    Record.AvailableWellChoices = WellChoiceMask(Plan.LumeProtocol);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::InheritedRouteSecured) |
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::DistrictPairIntegrated) |
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::BothEvidenceChannelsAttested) |
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::RecordedProtocolApplied) |
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::CoalitionRallied) |
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::OruunSurvived) |
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesNoNeutralLedgerCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedProtocol = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitFutureThatWonCompletion(
    echoes::sim::FutureWellChoice& OutRecordedProtocol,
    FString& OutFeedback)
{
    const FEchoesFutureThatWonPlan Plan = GetFutureThatWonPlan();
    OutRecordedProtocol = Plan.RecordedProtocol;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignFutureThatWon ||
        GetFutureThatWonPhase() != EEchoesFutureThatWonPhase::Complete ||
        !IsFutureThatWonUnlocked() ||
        Plan.RecordedProtocol == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative The Future That Won operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheFutureThatWon;
    Record.WellChoice = Plan.RecordedProtocol;
    Record.AvailableWellChoices = WellChoiceMask(Plan.RecordedProtocol);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::PriorElevenRecordLedgerConsumed) |
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::RecordedLumeProtocolBound) |
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::BothRecordedDistrictInputsVerified) |
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::IndependentPublicReadbackEstablished) |
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::RecordedProtocolActivated) |
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::StabilityWindowHeld) |
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::BothDistrictReadbacksObserved) |
        static_cast<uint8>(EEchoesFutureThatWonCompletionFact::LocalCoreSurvived);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedProtocol = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitAssemblyOfTheMissingCompletion(
    echoes::sim::FutureWellChoice& OutRecordedProtocol,
    FString& OutFeedback)
{
    const FEchoesAssemblyOfTheMissingPlan Plan =
        GetAssemblyOfTheMissingPlan();
    OutRecordedProtocol = Plan.RecordedProtocol;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignAssemblyOfTheMissing ||
        GetAssemblyOfTheMissingPhase() !=
            EEchoesAssemblyOfTheMissingPhase::Complete ||
        !IsAssemblyOfTheMissingUnlocked() ||
        Plan.RecordedProtocol == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Assembly of the Missing operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::AssemblyOfTheMissing;
    Record.WellChoice = Plan.RecordedProtocol;
    Record.AvailableWellChoices = WellChoiceMask(Plan.RecordedProtocol);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::PriorTwelveRecordLedgerConsumed) |
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::ExistingPlanProjectionBound) |
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::RecordedLumeProtocolBound) |
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::PriorPublicReceiptsBound) |
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::PublicRecordReadbackEstablished) |
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::CrownfallIndexLinked) |
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::IndependentAssemblyObserved) |
        static_cast<uint8>(EEchoesAssemblyOfTheMissingCompletionFact::LocalCoreSurvived);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedProtocol = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitSeveralVoicesOneCommandCompletion(
    echoes::sim::FutureWellChoice& OutRecordedProtocol,
    FString& OutFeedback)
{
    const FEchoesSeveralVoicesOneCommandPlan Plan =
        GetSeveralVoicesOneCommandPlan();
    OutRecordedProtocol = Plan.RecordedProtocol;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand ||
        GetSeveralVoicesOneCommandPhase() !=
            EEchoesSeveralVoicesOneCommandPhase::Complete ||
        !IsSeveralVoicesOneCommandUnlocked() ||
        Plan.RecordedProtocol == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Several Voices, One Command operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::SeveralVoicesOneCommand;
    Record.WellChoice = Plan.RecordedProtocol;
    Record.AvailableWellChoices = WellChoiceMask(Plan.RecordedProtocol);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::PriorThirteenRecordLedgerConsumed) |
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::ChoirCommandAuthorityEstablished) |
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::HeldAlternativesResearched) |
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::IncompatibleVoicesResolved) |
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::SharedResolutionResearched) |
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::PhaseAnchorRaised) |
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::CrisisWindowHeld) |
        static_cast<uint8>(EEchoesSeveralVoicesOneCommandCompletionFact::LocalCoreSurvived);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedProtocol = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitBrokenSunCompletion(
    EEchoesFinalResolution& OutRecordedResolution,
    FString& OutFeedback)
{
    const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
    OutRecordedResolution = SelectedBrokenSunResolution;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT(
            "[CAMPAIGN_LEDGER_UNAVAILABLE] The final operation is complete, but no campaign ending was saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignTheBrokenSun ||
        GetBrokenSunPhase() != EEchoesBrokenSunPhase::Complete ||
        !IsBrokenSunUnlocked() ||
        !FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            Plan,
            SelectedBrokenSunResolution) ||
        BrokenSunApproachAnchorId == 0 ||
        BrokenSunResolutionConduitId == 0 ||
        !bBrokenSunResolutionHoldStarted ||
        bBrokenSunResolutionContractFailed)
    {
        OutFeedback = TEXT(
            "[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative The Broken Sun operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheBrokenSun;
    Record.WellChoice = Plan.RecordedProtocol;
    Record.AvailableWellChoices = WellChoiceMask(Plan.RecordedProtocol);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::PriorFourteenRecordLedgerConsumed) |
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::CrownfallApproachSecured) |
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::AccordAssemblyEstablished) |
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::FinalResolutionCommitted) |
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::ResolutionConduitRaised) |
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::ResolutionWindowHeld) |
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::NamedWitnessesSurvived) |
        static_cast<uint8>(EEchoesBrokenSunCompletionFact::LocalCoreSurvived);
    Record.FinalResolution = SelectedBrokenSunResolution;
    Record.AvailableFinalResolutions = Plan.AvailableFinalResolutions;
    Record.FinalPlanKey = Plan.StablePlanKey;
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedResolution = Existing->FinalResolution;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

void UEchoesSimulationSubsystem::AdvancePrologueCompletionPresentation()
{
    if (!bPrologueCompletionPresentationScenario ||
        PrologueCompletionPresentationStage < 0 ||
        !bScenarioReady || !Simulation.IsValid())
    {
        return;
    }

    FString Feedback;
    bool bCommandAccepted = false;
    const auto FailFixture = [this, &Feedback](const TCHAR* Stage)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_FAILED] stage=%s detail=%s"),
            Stage,
            Feedback.IsEmpty() ? TEXT("command rejected") : *Feedback);
        PrologueCompletionPresentationStage = -1;
        bSimulationPaused = true;
    };

    switch (PrologueCompletionPresentationStage)
    {
        case 0:
            bCommandAccepted = IssueCommand(
                echoes::sim::CommandType::Move,
                ArchiveCarrierId,
                0,
                SimToWorld(GetArchiveRecoverySite()),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback);
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("recover_archive"));
                return;
            }
            PrologueCompletionPresentationStage = 1;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=recover_archive command=ordinary_move"));
            return;

        case 1:
            if (GetProloguePhase() !=
                EEchoesProloguePhase::DecideFutureWell)
            {
                return;
            }
            bCommandAccepted = IssueCommand(
                echoes::sim::CommandType::Move,
                ProloguePresentationWorkerId,
                0,
                SimToWorld(Vec2::FromTiles(29, 29)),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback);
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("approach_well"));
                return;
            }
            PrologueCompletionPresentationStage = 2;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=decide_well command=ordinary_move protocol=Preserve"));
            return;

        case 2:
            if (!Simulation->IsEntityVisibleTo(
                    LocalPlayerId,
                    ProloguePresentationWellId))
            {
                return;
            }
            if (const echoes::sim::Entity* Well =
                    Simulation->FindEntity(ProloguePresentationWellId))
            {
                bCommandAccepted = IssueCommand(
                    echoes::sim::CommandType::FutureWell,
                    ProloguePresentationWorkerId,
                    ProloguePresentationWellId,
                    SimToWorld(Well->position),
                    echoes::sim::FutureWellChoice::Preserve,
                    Feedback);
            }
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("commit_preserve"));
                return;
            }
            PrologueCompletionPresentationStage = 3;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=preserve command=ordinary_future_well"));
            return;

        case 3:
            if (GetProloguePhase() != EEchoesProloguePhase::Withdraw)
            {
                return;
            }
            bCommandAccepted = IssueCommand(
                echoes::sim::CommandType::Move,
                ArchiveCarrierId,
                0,
                SimToWorld(GetEvacuationSite()),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback);
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("withdraw"));
                return;
            }
            PrologueCompletionPresentationStage = 4;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=withdraw command=ordinary_move"));
            return;

        case 4:
            if (GetProloguePhase() == EEchoesProloguePhase::Complete &&
                bMatchResultReported)
            {
                PrologueCompletionPresentationStage = 5;
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_COMPLETE] phase=complete resultPresented=true ledgerRecords=%d controlled=true release=false"),
                    CampaignProgress.Decisions.Num());
            }
            return;

        default:
            return;
    }
}

bool UEchoesSimulationSubsystem::QuickSaveScenario(FString& OutFeedback) const
{
    OutFeedback.Reset();
    if (!bScenarioReady || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[SAVE_SIM_NOT_READY] No active scenario can be saved.");
        return false;
    }
    if (bStressScenario)
    {
        OutFeedback = TEXT(
            "[SAVE_STRESS_DISABLED] Stress fixtures cannot overwrite player checkpoints.");
        return false;
    }

    const std::vector<uint8> Snapshot = Simulation->SaveSnapshot();
    if (Snapshot.empty() || Snapshot.size() > MAX_int32)
    {
        OutFeedback = TEXT("[SAVE_SNAPSHOT_INVALID] The deterministic snapshot could not be created.");
        return false;
    }
    TArray<uint8> SnapshotBytes;
    SnapshotBytes.Append(Snapshot.data(), static_cast<int32>(Snapshot.size()));
    TArray<uint8> PersistedBytes = SnapshotBytes;
    if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun)
    {
        FString EnvelopeError;
        if (!BuildBrokenSunQuickSaveEnvelope(
                CampaignProgress,
                SnapshotBytes,
                PendingBrokenSunResolution,
                SelectedBrokenSunResolution,
                bBrokenSunResolutionHoldStarted,
                bBrokenSunResolutionContractFailed,
                BrokenSunResolutionStartTick,
                BrokenSunApproachAnchorId,
                BrokenSunResolutionConduitId,
                PersistedBytes,
                EnvelopeError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
                *EnvelopeError);
            return false;
        }
    }
    else if (SelectedOperation ==
        EEchoesOperationMode::CampaignChoirAtLumeReach)
    {
        FString EnvelopeError;
        if (!BuildChoirAtLumeReachQuickSaveEnvelope(
                CampaignProgress,
                SnapshotBytes,
                PersistedBytes,
                EnvelopeError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
                *EnvelopeError);
            return false;
        }
    }
    else if (SelectedOperation ==
             EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
    {
        FString EnvelopeError;
        if (!BuildSeveralVoicesOneCommandQuickSaveEnvelope(
                CampaignProgress,
                SnapshotBytes,
                bSeveralVoicesCrisisHoldStarted,
                bSeveralVoicesCrisisContractFailed,
                PersistedBytes,
                EnvelopeError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
                *EnvelopeError);
            return false;
        }
    }
    else if (SelectedOperation ==
             EEchoesOperationMode::CampaignFutureThatWon)
    {
        FString EnvelopeError;
        if (!BuildFutureThatWonQuickSaveEnvelope(
                CampaignProgress,
                SnapshotBytes,
                PersistedBytes,
                EnvelopeError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
                *EnvelopeError);
            return false;
        }
    }
    else if (SelectedOperation ==
             EEchoesOperationMode::CampaignAssemblyOfTheMissing)
    {
        FString EnvelopeError;
        if (!BuildAssemblyOfTheMissingQuickSaveEnvelope(
                CampaignProgress,
                SnapshotBytes,
                PersistedBytes,
                EnvelopeError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
                *EnvelopeError);
            return false;
        }
    }
    else if (SelectedOperation ==
             EEchoesOperationMode::CampaignNoNeutralLedger)
    {
        FString EnvelopeError;
        if (!BuildNoNeutralLedgerQuickSaveEnvelope(
                CampaignProgress,
                SnapshotBytes,
                PersistedBytes,
                EnvelopeError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
                *EnvelopeError);
            return false;
        }
    }
    if (UsesQuickSaveContainer(SelectedOperation))
    {
        uint64 CampaignBranchIdentity = 0;
        FString BranchIdentityError;
        if (!BuildQuickSaveBranchIdentity(
                SelectedOperation,
                CampaignProgress,
                CampaignBranchIdentity,
                BranchIdentityError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
                *BranchIdentityError);
            return false;
        }
        TArray<uint8> ContainerBytes;
        FString ContainerError;
        if (!BuildQuickSaveContainer(
                SelectedOperation,
                LocalFaction,
                CampaignBranchIdentity,
                PersistedBytes,
                ContainerBytes,
                ContainerError))
        {
            OutFeedback = FString::Printf(
                TEXT("[SAVE_CONTAINER_FAILED] %s"),
                *ContainerError);
            return false;
        }
        PersistedBytes = MoveTemp(ContainerBytes);
    }

    const FString SavePath = GetActiveQuickSavePath();
    const FString SaveDirectory = FPaths::GetPath(SavePath);
    const FString TemporaryPath = SavePath + TEXT(".tmp");
    const FString BackupPath = SavePath + TEXT(".bak");
    const FString BackupTemporaryPath = BackupPath + TEXT(".tmp");
    IFileManager& Files = IFileManager::Get();
    if (!Files.MakeDirectory(*SaveDirectory, true))
    {
        OutFeedback = TEXT("[SAVE_DIRECTORY_FAILED] The save directory could not be created.");
        return false;
    }
    Files.Delete(*TemporaryPath, false, true, true);
    if (!FFileHelper::SaveArrayToFile(PersistedBytes, *TemporaryPath))
    {
        OutFeedback = TEXT("[SAVE_WRITE_FAILED] The temporary checkpoint could not be written.");
        return false;
    }

    uint64 ExpectedCampaignBranchIdentity = 0;
    FString BranchIdentityError;
    if (!BuildQuickSaveBranchIdentity(
            SelectedOperation,
            CampaignProgress,
            ExpectedCampaignBranchIdentity,
            BranchIdentityError))
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = FString::Printf(
            TEXT("[SAVE_LEDGER_BINDING_FAILED] %s"),
            *BranchIdentityError);
        return false;
    }

    const auto ValidateCheckpointFile =
        [this, ExpectedCampaignBranchIdentity](
                                                const FString& CandidatePath,
                                                FString& OutFailure)
    {
        TArray<uint8> CandidateBytes;
        if (!FFileHelper::LoadFileToArray(CandidateBytes, *CandidatePath))
        {
            OutFailure = TEXT("file unavailable");
            return false;
        }

        TArray<uint8> ContainerPayload;
        const TArray<uint8>* OperationPayload = &CandidateBytes;
        if (UsesQuickSaveContainer(SelectedOperation))
        {
            const EQuickSaveContainerRead ContainerRead =
                ExtractQuickSaveContainer(
                    SelectedOperation,
                    LocalFaction,
                    ExpectedCampaignBranchIdentity,
                    CandidateBytes,
                    ContainerPayload,
                    OutFailure);
            if (ContainerRead == EQuickSaveContainerRead::Invalid)
            {
                return false;
            }
            if (ContainerRead == EQuickSaveContainerRead::Wrapped)
            {
                OperationPayload = &ContainerPayload;
            }
        }

        TArray<uint8> SnapshotBytes;
        const TArray<uint8>* SnapshotPayload = OperationPayload;
        bool bCrisisHoldStarted = false;
        bool bCrisisContractFailed = false;
        EEchoesFinalResolution PendingResolution =
            EEchoesFinalResolution::None;
        EEchoesFinalResolution SelectedResolution =
            EEchoesFinalResolution::None;
        bool bResolutionHoldStarted = false;
        bool bResolutionContractFailed = false;
        uint64 ResolutionStartTick = 0;
        EntityId ApproachAnchorId = 0;
        EntityId ResolutionConduitId = 0;
        bool bEnvelopeValid = true;
        if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun)
        {
            bEnvelopeValid = ExtractBrokenSunQuickSaveSnapshot(
                CampaignProgress,
                *OperationPayload,
                PendingResolution,
                SelectedResolution,
                bResolutionHoldStarted,
                bResolutionContractFailed,
                ResolutionStartTick,
                ApproachAnchorId,
                ResolutionConduitId,
                SnapshotBytes,
                OutFailure);
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignChoirAtLumeReach)
        {
            bEnvelopeValid = ExtractChoirAtLumeReachQuickSaveSnapshot(
                CampaignProgress,
                *OperationPayload,
                SnapshotBytes,
                OutFailure);
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            bEnvelopeValid = ExtractNoNeutralLedgerQuickSaveSnapshot(
                CampaignProgress,
                *OperationPayload,
                SnapshotBytes,
                OutFailure);
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignFutureThatWon)
        {
            bEnvelopeValid = ExtractFutureThatWonQuickSaveSnapshot(
                CampaignProgress,
                *OperationPayload,
                SnapshotBytes,
                OutFailure);
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            bEnvelopeValid = ExtractAssemblyOfTheMissingQuickSaveSnapshot(
                CampaignProgress,
                *OperationPayload,
                SnapshotBytes,
                OutFailure);
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            bEnvelopeValid = ExtractSeveralVoicesOneCommandQuickSaveSnapshot(
                CampaignProgress,
                *OperationPayload,
                bCrisisHoldStarted,
                bCrisisContractFailed,
                SnapshotBytes,
                OutFailure);
            SnapshotPayload = &SnapshotBytes;
        }
        if (!bEnvelopeValid)
        {
            return false;
        }

        std::string SnapshotError;
        std::optional<echoes::sim::Simulation> Candidate =
            echoes::sim::Simulation::LoadSnapshot(
                std::span<const uint8>(
                    SnapshotPayload->GetData(),
                    static_cast<size_t>(SnapshotPayload->Num())),
                &SnapshotError);
        if (!Candidate.has_value())
        {
            OutFailure = SnapshotError.empty()
                ? TEXT("checkpoint snapshot could not be reopened")
                : UTF8_TO_TCHAR(SnapshotError.c_str());
            return false;
        }
        const echoes::sim::SimulationConfig& Config = Candidate->Config();
        const echoes::sim::PlayerState* CandidateLocalPlayer =
            Candidate->FindPlayer(LocalPlayerId);
        if (Config.mapWidthTiles != PrototypeMapWidthTiles ||
            Config.mapHeightTiles != PrototypeMapHeightTiles ||
            Config.ticksPerSecond != PrototypeTicksPerSecond ||
            Config.randomSeed != PrototypeSeed ||
            Config.protectedCommandCorePlayerMask != 0 ||
            !Candidate->NextCommandSequence(LocalPlayerId).has_value() ||
            CandidateLocalPlayer == nullptr ||
            CandidateLocalPlayer->faction != LocalFaction)
        {
            OutFailure = TEXT(
                "checkpoint is not compatible with the active Glass Scar operation and faction");
            return false;
        }
        return true;
    };

    FString TemporaryFailure;
    if (!ValidateCheckpointFile(TemporaryPath, TemporaryFailure))
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = FString::Printf(
            TEXT("[SAVE_VALIDATION_FAILED] %s"),
            TemporaryFailure.IsEmpty()
                ? TEXT("The temporary checkpoint could not be reopened.")
                : *TemporaryFailure);
        return false;
    }

    const bool bHadPriorSave = Files.FileExists(*SavePath);
    FString PriorFailure;
    const bool bPriorSaveValid =
        bHadPriorSave && ValidateCheckpointFile(SavePath, PriorFailure);
    const bool bHadPriorBackup = Files.FileExists(*BackupPath);
    FString ExistingBackupFailure;
    const bool bExistingBackupValid =
        bHadPriorBackup &&
        ValidateCheckpointFile(BackupPath, ExistingBackupFailure);
    const bool bHadStagedBackup =
        Files.FileExists(*BackupTemporaryPath);
    FString ExistingStagedBackupFailure;
    const bool bExistingStagedBackupValid =
        bHadStagedBackup &&
        ValidateCheckpointFile(
            BackupTemporaryPath,
            ExistingStagedBackupFailure);
    bool bRetainedValidPrimary = false;
    bool bBackupRotationDeferred = false;
    if (bPriorSaveValid)
    {
        TArray<uint8> PriorPrimaryBytes;
        if (!FFileHelper::LoadFileToArray(PriorPrimaryBytes, *SavePath) ||
            PriorPrimaryBytes.IsEmpty() ||
            !FFileHelper::SaveArrayToFile(
                PriorPrimaryBytes,
                *BackupTemporaryPath))
        {
            Files.Delete(*TemporaryPath, false, true, true);
            Files.Delete(*BackupTemporaryPath, false, true, true);
            OutFeedback = TEXT(
                "[SAVE_BACKUP_FAILED] The prior validated checkpoint could not be staged safely.");
            return false;
        }
        FString StagedBackupFailure;
        if (!ValidateCheckpointFile(
                BackupTemporaryPath,
                StagedBackupFailure))
        {
            Files.Delete(*TemporaryPath, false, true, true);
            Files.Delete(*BackupTemporaryPath, false, true, true);
            OutFeedback = FString::Printf(
                TEXT("[SAVE_BACKUP_VALIDATION_FAILED] %s"),
                StagedBackupFailure.IsEmpty()
                    ? TEXT("The staged recovery checkpoint could not be reopened.")
                    : *StagedBackupFailure);
            return false;
        }
    }
    else if (bHadPriorSave)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_QUICK_SAVE_PRIMARY_REJECTED] detail=%s validRecoveryPreserved=%s backupDetail=%s stagedDetail=%s"),
            PriorFailure.IsEmpty() ? TEXT("checkpoint invalid") : *PriorFailure,
            bExistingBackupValid || bExistingStagedBackupValid
                ? TEXT("true")
                : TEXT("false"),
            bHadPriorBackup
                ? (ExistingBackupFailure.IsEmpty()
                       ? TEXT("validated")
                       : *ExistingBackupFailure)
                : TEXT("not present"),
            bHadStagedBackup
                ? (ExistingStagedBackupFailure.IsEmpty()
                       ? TEXT("validated")
                       : *ExistingStagedBackupFailure)
                : TEXT("not present"));
    }
    if (!AtomicReplaceFile(SavePath, TemporaryPath))
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = bPriorSaveValid
            ? TEXT("[SAVE_COMMIT_FAILED] The validated checkpoint was not committed; the prior checkpoint remains active and its recovery copy remains staged.")
            : TEXT("[SAVE_COMMIT_FAILED] The validated checkpoint was not committed.");
        return false;
    }
    if (bPriorSaveValid)
    {
        bool bForceBackupRotationFailure = false;
#if WITH_DEV_AUTOMATION_TESTS
        bForceBackupRotationFailure =
            bFailNextQuickSaveBackupRotationForTesting;
        bFailNextQuickSaveBackupRotationForTesting = false;
#endif
        if (bForceBackupRotationFailure ||
            !AtomicReplaceFile(BackupPath, BackupTemporaryPath))
        {
            bBackupRotationDeferred = true;
            OutFeedback = TEXT(
                "QUICK SAVE: the new primary committed; backup rotation was deferred and the prior validated checkpoint remains staged for recovery.");
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_QUICK_SAVE_BACKUP_ROTATION_DEFERRED] stagedRecovery=%s existingBackupValid=%s"),
                *BackupTemporaryPath,
                bExistingBackupValid ? TEXT("true") : TEXT("false"));
        }
        else
        {
            bRetainedValidPrimary = true;
        }
    }

    const unsigned long long CurrentTick =
        static_cast<unsigned long long>(Simulation->CurrentTick());
    if (!OutFeedback.IsEmpty())
    {
        // Preserve the explicit degraded-recovery result above.
    }
    else if (bHadPriorSave && !bPriorSaveValid && bExistingBackupValid)
    {
        OutFeedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed; the existing validated recovery checkpoint was preserved."),
            CurrentTick);
    }
    else if (bHadPriorSave && !bPriorSaveValid &&
             bExistingStagedBackupValid)
    {
        OutFeedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed; the validated staged recovery checkpoint was preserved."),
            CurrentTick);
    }
    else if (bHadPriorSave && !bPriorSaveValid &&
             (bHadPriorBackup || bHadStagedBackup))
    {
        OutFeedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed; the prior primary and recovery generations were invalid, so this checkpoint is the only validated generation."),
            CurrentTick);
    }
    else if (bHadPriorSave && !bPriorSaveValid)
    {
        OutFeedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed; the prior primary was invalid and no validated recovery generation existed, so this checkpoint is the only validated generation."),
            CurrentTick);
    }
    else
    {
        OutFeedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed."),
            CurrentTick);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_QUICK_SAVE] tick=%llu bytes=%d backup=%s ledgerBound=%s"),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        PersistedBytes.Num(),
        bRetainedValidPrimary
            ? TEXT("rotated")
        : bBackupRotationDeferred
            ? TEXT("staged_rotation_deferred")
        : bExistingBackupValid
            ? TEXT("preserved_valid")
        : bExistingStagedBackupValid
            ? TEXT("preserved_valid_staged")
        : bHadPriorBackup
            ? TEXT("preserved_invalid")
        : bHadStagedBackup
            ? TEXT("preserved_invalid_staged")
        : bHadPriorSave ? TEXT("none_invalid_primary") : TEXT("none"),
        SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun ||
            RequiresCampaignBranchBoundQuickSave(SelectedOperation) ||
            SelectedOperation ==
                EEchoesOperationMode::CampaignChoirAtLumeReach ||
            SelectedOperation ==
                EEchoesOperationMode::CampaignNoNeutralLedger ||
            SelectedOperation ==
                EEchoesOperationMode::CampaignFutureThatWon ||
            SelectedOperation ==
                EEchoesOperationMode::CampaignAssemblyOfTheMissing ||
            SelectedOperation ==
                EEchoesOperationMode::CampaignSeveralVoicesOneCommand
            ? TEXT("true")
            : TEXT("false"));
    return true;
}

bool UEchoesSimulationSubsystem::QuickLoadScenario(FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!bScenarioReady || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[LOAD_SIM_NOT_READY] Start a scenario before loading.");
        return false;
    }
    if (bStressScenario)
    {
        OutFeedback = TEXT(
            "[LOAD_STRESS_DISABLED] Stress fixtures cannot consume player checkpoints.");
        return false;
    }

    const FString SavePath = GetActiveQuickSavePath();
    const FString BackupPath = SavePath + TEXT(".bak");
    const FString BackupTemporaryPath = BackupPath + TEXT(".tmp");
    uint64 ExpectedCampaignBranchIdentity = 0;
    FString BranchIdentityError;
    if (!BuildQuickSaveBranchIdentity(
            SelectedOperation,
            CampaignProgress,
            ExpectedCampaignBranchIdentity,
            BranchIdentityError))
    {
        OutFeedback = FString::Printf(
            TEXT("[LOAD_LEDGER_BINDING_FAILED] %s"),
            *BranchIdentityError);
        return false;
    }
    TUniquePtr<echoes::sim::Simulation> LoadedSimulation;
    bool bLoadedCrisisHoldStarted = false;
    bool bLoadedCrisisContractFailed = false;
    EEchoesFinalResolution LoadedPendingResolution =
        EEchoesFinalResolution::None;
    EEchoesFinalResolution LoadedSelectedResolution =
        EEchoesFinalResolution::None;
    bool bLoadedResolutionHoldStarted = false;
    bool bLoadedResolutionContractFailed = false;
    uint64 LoadedResolutionStartTick = 0;
    EntityId LoadedApproachAnchorId = 0;
    EntityId LoadedResolutionConduitId = 0;
    FString SelectedPath;
    FString PrimaryFailure;
    const auto TryLoad = [this,
                          &LoadedSimulation,
                          &bLoadedCrisisHoldStarted,
                          &bLoadedCrisisContractFailed,
                          &LoadedPendingResolution,
                          &LoadedSelectedResolution,
                          &bLoadedResolutionHoldStarted,
                          &bLoadedResolutionContractFailed,
                          &LoadedResolutionStartTick,
                          &LoadedApproachAnchorId,
                          &LoadedResolutionConduitId,
                          ExpectedCampaignBranchIdentity](
                             const FString& CandidatePath,
                             FString& OutFailure)
    {
        TArray<uint8> Bytes;
        if (!IFileManager::Get().FileExists(*CandidatePath))
        {
            OutFailure = TEXT("file unavailable");
            return false;
        }
        if (!FFileHelper::LoadFileToArray(Bytes, *CandidatePath))
        {
            OutFailure = TEXT("file unavailable");
            return false;
        }
        TArray<uint8> ContainerPayload;
        const TArray<uint8>* OperationPayload = &Bytes;
        if (UsesQuickSaveContainer(SelectedOperation))
        {
            const EQuickSaveContainerRead ContainerRead =
                ExtractQuickSaveContainer(
                    SelectedOperation,
                    LocalFaction,
                    ExpectedCampaignBranchIdentity,
                    Bytes,
                    ContainerPayload,
                    OutFailure);
            if (ContainerRead == EQuickSaveContainerRead::Invalid)
            {
                return false;
            }
            if (ContainerRead == EQuickSaveContainerRead::Wrapped)
            {
                OperationPayload = &ContainerPayload;
            }
        }
        TArray<uint8> SnapshotBytes;
        const TArray<uint8>* SnapshotPayload = OperationPayload;
        bool bCandidateCrisisHoldStarted = false;
        bool bCandidateCrisisContractFailed = false;
        EEchoesFinalResolution CandidatePendingResolution =
            EEchoesFinalResolution::None;
        EEchoesFinalResolution CandidateSelectedResolution =
            EEchoesFinalResolution::None;
        bool bCandidateResolutionHoldStarted = false;
        bool bCandidateResolutionContractFailed = false;
        uint64 CandidateResolutionStartTick = 0;
        EntityId CandidateApproachAnchorId = 0;
        EntityId CandidateResolutionConduitId = 0;
        if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun)
        {
            if (!ExtractBrokenSunQuickSaveSnapshot(
                    CampaignProgress,
                    *OperationPayload,
                    CandidatePendingResolution,
                    CandidateSelectedResolution,
                    bCandidateResolutionHoldStarted,
                    bCandidateResolutionContractFailed,
                    CandidateResolutionStartTick,
                    CandidateApproachAnchorId,
                    CandidateResolutionConduitId,
                    SnapshotBytes,
                    OutFailure))
            {
                return false;
            }
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach)
        {
            if (!ExtractChoirAtLumeReachQuickSaveSnapshot(
                    CampaignProgress,
                    *OperationPayload,
                    SnapshotBytes,
                    OutFailure))
            {
                return false;
            }
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            if (!ExtractAssemblyOfTheMissingQuickSaveSnapshot(
                    CampaignProgress,
                    *OperationPayload,
                    SnapshotBytes,
                    OutFailure))
            {
                return false;
            }
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            if (!ExtractSeveralVoicesOneCommandQuickSaveSnapshot(
                    CampaignProgress,
                    *OperationPayload,
                    bCandidateCrisisHoldStarted,
                    bCandidateCrisisContractFailed,
                    SnapshotBytes,
                    OutFailure))
            {
                return false;
            }
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            if (!ExtractNoNeutralLedgerQuickSaveSnapshot(
                    CampaignProgress,
                    *OperationPayload,
                    SnapshotBytes,
                    OutFailure))
            {
                return false;
            }
            SnapshotPayload = &SnapshotBytes;
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignFutureThatWon)
        {
            if (!ExtractFutureThatWonQuickSaveSnapshot(
                    CampaignProgress,
                    *OperationPayload,
                    SnapshotBytes,
                    OutFailure))
            {
                return false;
            }
            SnapshotPayload = &SnapshotBytes;
        }
        std::string Error;
        std::optional<echoes::sim::Simulation> Candidate =
            echoes::sim::Simulation::LoadSnapshot(
                std::span<const uint8>(
                    SnapshotPayload->GetData(),
                    static_cast<size_t>(SnapshotPayload->Num())),
                &Error);
        if (!Candidate.has_value())
        {
            OutFailure = UTF8_TO_TCHAR(Error.c_str());
            return false;
        }
        const echoes::sim::SimulationConfig& Config = Candidate->Config();
        if (Config.mapWidthTiles != PrototypeMapWidthTiles ||
            Config.mapHeightTiles != PrototypeMapHeightTiles ||
            Config.ticksPerSecond != PrototypeTicksPerSecond ||
            Config.randomSeed != PrototypeSeed ||
            Config.protectedCommandCorePlayerMask != 0 ||
            !Candidate->NextCommandSequence(LocalPlayerId).has_value() ||
            Candidate->FindPlayer(LocalPlayerId) == nullptr ||
            Candidate->FindPlayer(LocalPlayerId)->faction != LocalFaction)
        {
            OutFailure = TEXT("snapshot is not a compatible Glass Scar scenario");
            return false;
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            const FEchoesTermsOfContinuancePlan Plan =
                GetTermsOfContinuancePlan();
            const echoes::sim::Entity* MeridianRelay =
                Candidate->FindEntity(MeridianContinuanceRelayId);
            const echoes::sim::Entity* KharuunSpine =
                Candidate->FindEntity(KharuunContinuanceSpineId);
            const echoes::sim::Entity* MeridianWitness =
                Candidate->FindEntity(MeridianContinuanceWitnessId);
            const echoes::sim::Entity* KharuunWitness =
                Candidate->FindEntity(KharuunContinuanceWitnessId);
            const auto IsMeridianProxy = [](const echoes::sim::Entity* Entity,
                                            echoes::sim::EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction ==
                           echoes::sim::Faction::MeridianCompact &&
                       Entity->type == Type;
            };
            if (!IsMeridianProxy(
                    MeridianRelay,
                    echoes::sim::EntityType::UtilityStructure) ||
                !IsMeridianProxy(
                    KharuunSpine,
                    echoes::sim::EntityType::UtilityStructure) ||
                !IsMeridianProxy(
                    MeridianWitness,
                    echoes::sim::EntityType::ScoutUnit) ||
                !IsMeridianProxy(
                    KharuunWitness,
                    echoes::sim::EntityType::ScoutUnit) ||
                MeridianRelay->position != Plan.MeridianRelaySite ||
                KharuunSpine->position != Plan.KharuunSpineSite)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active Terms of Continuance ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            const FEchoesNamesWithoutBirthsPlan Plan =
                GetNamesWithoutBirthsPlan();
            const echoes::sim::Entity* Talar =
                Candidate->FindEntity(TalarId);
            const echoes::sim::Entity* Archive =
                Candidate->FindEntity(CensusArchiveId);
            const echoes::sim::Entity* FirstCivilian =
                Candidate->FindEntity(FirstCivilianId);
            const echoes::sim::Entity* SecondCivilian =
                Candidate->FindEntity(SecondCivilianId);
            const auto IsMeridianProxy = [](const echoes::sim::Entity* Entity,
                                            EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::MeridianCompact &&
                       Entity->type == Type;
            };
            if (!IsMeridianProxy(Talar, EntityType::ScoutUnit) ||
                !IsMeridianProxy(Archive, EntityType::UtilityStructure) ||
                !IsMeridianProxy(FirstCivilian, EntityType::Worker) ||
                !IsMeridianProxy(SecondCivilian, EntityType::Worker) ||
                Archive->position != Plan.CensusSite)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active Names Without Births ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence)
        {
            const echoes::sim::Entity* Waystone =
                Candidate->FindEntity(MigrationWaystoneId);
            const echoes::sim::Entity* Oruun =
                Candidate->FindEntity(OruunId);
            const echoes::sim::Entity* FirstWitness =
                Candidate->FindEntity(FirstMemoryWitnessId);
            const echoes::sim::Entity* SecondWitness =
                Candidate->FindEntity(SecondMemoryWitnessId);
            const auto IsKharuunProxy = [](const echoes::sim::Entity* Entity,
                                           EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::KharuunAssemblies &&
                       Entity->type == Type;
            };
            if (!IsKharuunProxy(Waystone, EntityType::Dropoff) ||
                !IsKharuunProxy(Oruun, EntityType::ScoutUnit) ||
                !IsKharuunProxy(FirstWitness, EntityType::ScoutUnit) ||
                !IsKharuunProxy(SecondWitness, EntityType::ScoutUnit) ||
                OruunId == FirstMemoryWitnessId ||
                OruunId == SecondMemoryWitnessId ||
                FirstMemoryWitnessId == SecondMemoryWitnessId)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active The Shape of Silence ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs)
        {
            const echoes::sim::Entity* Talar =
                Candidate->FindEntity(ShapeBesideUsTalarId);
            const echoes::sim::Entity* FirstWitness =
                Candidate->FindEntity(FirstStateWitnessId);
            const echoes::sim::Entity* SecondWitness =
                Candidate->FindEntity(SecondStateWitnessId);
            const auto IsMeridianProxy = [](const echoes::sim::Entity* Entity,
                                            EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::MeridianCompact &&
                       Entity->type == Type;
            };
            if (!IsMeridianProxy(Talar, EntityType::ScoutUnit) ||
                !IsMeridianProxy(FirstWitness, EntityType::Worker) ||
                !IsMeridianProxy(SecondWitness, EntityType::Soldier) ||
                ShapeBesideUsTalarId == FirstStateWitnessId ||
                ShapeBesideUsTalarId == SecondStateWitnessId ||
                FirstStateWitnessId == SecondStateWitnessId)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active The Shape Beside Us ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority)
        {
            const echoes::sim::Entity* Mara =
                Candidate->FindEntity(ReserveAuthorityMaraId);
            const echoes::sim::Entity* LifeSupport =
                Candidate->FindEntity(LifeSupportDistrictId);
            const echoes::sim::Entity* Transit =
                Candidate->FindEntity(TransitDistrictId);
            const echoes::sim::Entity* Archive =
                Candidate->FindEntity(ArchiveDistrictId);
            const auto IsMeridianProxy = [](const echoes::sim::Entity* Entity,
                                            EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::MeridianCompact &&
                       Entity->type == Type;
            };
            if (!IsMeridianProxy(Mara, EntityType::ScoutUnit) ||
                !IsMeridianProxy(
                    LifeSupport, EntityType::UtilityStructure) ||
                !IsMeridianProxy(Transit, EntityType::UtilityStructure) ||
                !IsMeridianProxy(Archive, EntityType::UtilityStructure) ||
                ReserveAuthorityMaraId == LifeSupportDistrictId ||
                ReserveAuthorityMaraId == TransitDistrictId ||
                ReserveAuthorityMaraId == ArchiveDistrictId ||
                LifeSupportDistrictId == TransitDistrictId ||
                LifeSupportDistrictId == ArchiveDistrictId ||
                TransitDistrictId == ArchiveDistrictId ||
                LifeSupport->position !=
                    FEchoesCityReserveMissionModel::SiteForDistrict(
                        EEchoesCityDistrict::LifeSupport) ||
                Transit->position !=
                    FEchoesCityReserveMissionModel::SiteForDistrict(
                        EEchoesCityDistrict::Transit) ||
                Archive->position !=
                    FEchoesCityReserveMissionModel::SiteForDistrict(
                        EEchoesCityDistrict::Archive))
            {
                OutFailure = TEXT(
                    "snapshot does not match the active Reserve Authority ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach)
        {
            const FEchoesChoirAtLumeReachPlan Plan =
                GetChoirAtLumeReachPlan();
            const echoes::sim::Entity* Oruun =
                Candidate->FindEntity(ChoirAtLumeReachOruunId);
            const echoes::sim::Entity* Waystone =
                Candidate->FindEntity(ChoirAtLumeReachWaystoneId);
            const echoes::sim::Entity* Well =
                Candidate->FindEntity(ChoirAtLumeReachWellId);
            const auto IsKharuunEntity = [](const echoes::sim::Entity* Entity,
                                            EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::KharuunAssemblies &&
                       Entity->type == Type;
            };
            const bool bValidWell =
                Well != nullptr && Well->type == EntityType::FutureWell &&
                (Well->owner == echoes::sim::kNeutralPlayer ||
                 Well->owner == LocalPlayerId) &&
                Well->position == Plan.FutureWellSite;
            if (!IsKharuunEntity(Oruun, EntityType::ScoutUnit) ||
                !IsKharuunEntity(Waystone, EntityType::Dropoff) ||
                !bValidWell ||
                ChoirAtLumeReachOruunId == ChoirAtLumeReachWaystoneId ||
                ChoirAtLumeReachOruunId == ChoirAtLumeReachWellId ||
                ChoirAtLumeReachWaystoneId == ChoirAtLumeReachWellId)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active The Choir at Lume Reach ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            const FEchoesNoNeutralLedgerPlan Plan =
                GetNoNeutralLedgerPlan();
            const echoes::sim::Entity* Oruun =
                Candidate->FindEntity(NoNeutralOruunId);
            const echoes::sim::Entity* Waystone =
                Candidate->FindEntity(NoNeutralWaystoneId);
            const echoes::sim::Entity* Witness =
                Candidate->FindEntity(NoNeutralLedgerWitnessId);
            const echoes::sim::Entity* FirstDistrictInterface =
                Candidate->FindEntity(
                    NoNeutralFirstDistrictInterfaceId);
            const echoes::sim::Entity* SecondDistrictInterface =
                Candidate->FindEntity(
                    NoNeutralSecondDistrictInterfaceId);
            const echoes::sim::Entity* MeridianEvidenceInterface =
                Candidate->FindEntity(
                    NoNeutralMeridianEvidenceInterfaceId);
            const echoes::sim::Entity* KharuunEvidenceInterface =
                Candidate->FindEntity(
                    NoNeutralKharuunEvidenceInterfaceId);
            const echoes::sim::Entity* Well =
                Candidate->FindEntity(NoNeutralWellId);
            const auto IsKharuunEntity = [](const echoes::sim::Entity* Entity,
                                            EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::KharuunAssemblies &&
                       Entity->type == Type;
            };
            const bool bValidWell =
                Well != nullptr && Well->type == EntityType::FutureWell &&
                (Well->owner == echoes::sim::kNeutralPlayer ||
                 Well->owner == LocalPlayerId) &&
                Well->position == Plan.FutureWellSite &&
                (Well->wellChoice == FutureWellChoice::Dormant ||
                 Well->wellChoice == Plan.LumeProtocol);
            TSet<EntityId> CompositionIds;
            CompositionIds.Reserve(8);
            CompositionIds.Add(NoNeutralOruunId);
            CompositionIds.Add(NoNeutralWaystoneId);
            CompositionIds.Add(NoNeutralLedgerWitnessId);
            CompositionIds.Add(NoNeutralFirstDistrictInterfaceId);
            CompositionIds.Add(NoNeutralSecondDistrictInterfaceId);
            CompositionIds.Add(NoNeutralMeridianEvidenceInterfaceId);
            CompositionIds.Add(NoNeutralKharuunEvidenceInterfaceId);
            CompositionIds.Add(NoNeutralWellId);
            if (!IsKharuunEntity(Oruun, EntityType::ScoutUnit) ||
                !IsKharuunEntity(Waystone, EntityType::Dropoff) ||
                !IsKharuunEntity(Witness, EntityType::ScoutUnit) ||
                !IsPublicInterface(
                    FirstDistrictInterface,
                    Faction::MeridianCompact,
                    Plan.FirstDistrictSite,
                    true) ||
                !IsPublicInterface(
                    SecondDistrictInterface,
                    Faction::MeridianCompact,
                    Plan.SecondDistrictSite,
                    true) ||
                !IsPublicInterface(
                    MeridianEvidenceInterface,
                    Faction::MeridianCompact,
                    Plan.MeridianEvidenceSite,
                    true) ||
                !IsPublicInterface(
                    KharuunEvidenceInterface,
                    Faction::KharuunAssemblies,
                    Plan.KharuunEvidenceSite,
                    false) ||
                !bValidWell ||
                CompositionIds.Num() != 8)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active No Neutral Ledger composition");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon)
        {
            const FEchoesFutureThatWonPlan Plan = GetFutureThatWonPlan();
            const echoes::sim::Entity* Oruun =
                Candidate->FindEntity(FutureWonOruunId);
            const echoes::sim::Entity* Verifier =
                Candidate->FindEntity(FutureWonVerifierId);
            const echoes::sim::Entity* FirstDistrict =
                Candidate->FindEntity(FutureWonFirstDistrictInterfaceId);
            const echoes::sim::Entity* SecondDistrict =
                Candidate->FindEntity(FutureWonSecondDistrictInterfaceId);
            const echoes::sim::Entity* MeridianReadback =
                Candidate->FindEntity(
                    FutureWonMeridianReadbackInterfaceId);
            const echoes::sim::Entity* KharuunReadback =
                Candidate->FindEntity(
                    FutureWonKharuunReadbackInterfaceId);
            const echoes::sim::Entity* Demonstrator =
                Candidate->FindEntity(FutureWonDemonstratorInterfaceId);
            const echoes::sim::Entity* Well =
                Candidate->FindEntity(FutureWonWellId);
            const auto IsKharuunScout = [](const echoes::sim::Entity* Entity)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::KharuunAssemblies &&
                       Entity->type == EntityType::ScoutUnit;
            };
            const bool bValidWell =
                Well != nullptr && Well->type == EntityType::FutureWell &&
                (Well->owner == echoes::sim::kNeutralPlayer ||
                 Well->owner == LocalPlayerId) &&
                Well->position == Plan.FutureWellSite &&
                (Well->wellChoice == FutureWellChoice::Dormant ||
                 Well->wellChoice == Plan.RecordedProtocol);
            TSet<EntityId> CompositionIds;
            CompositionIds.Reserve(8);
            CompositionIds.Add(FutureWonOruunId);
            CompositionIds.Add(FutureWonVerifierId);
            CompositionIds.Add(FutureWonFirstDistrictInterfaceId);
            CompositionIds.Add(FutureWonSecondDistrictInterfaceId);
            CompositionIds.Add(FutureWonMeridianReadbackInterfaceId);
            CompositionIds.Add(FutureWonKharuunReadbackInterfaceId);
            CompositionIds.Add(FutureWonDemonstratorInterfaceId);
            CompositionIds.Add(FutureWonWellId);
            if (!IsKharuunScout(Oruun) || !IsKharuunScout(Verifier) ||
                !IsPublicInterface(
                    FirstDistrict,
                    Faction::MeridianCompact,
                    Plan.FirstDistrictInputSite,
                    true) ||
                !IsPublicInterface(
                    SecondDistrict,
                    Faction::MeridianCompact,
                    Plan.SecondDistrictInputSite,
                    true) ||
                !IsPublicInterface(
                    MeridianReadback,
                    Faction::MeridianCompact,
                    Plan.MeridianReadbackSite,
                    true) ||
                !IsPublicInterface(
                    KharuunReadback,
                    Faction::KharuunAssemblies,
                    Plan.KharuunReadbackSite,
                    false) ||
                !IsPublicInterface(
                    Demonstrator,
                    Faction::MeridianCompact,
                    Plan.RestorationDemonstratorSite,
                    true) ||
                !bValidWell || CompositionIds.Num() != 8)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active The Future That Won composition");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            const FEchoesAssemblyOfTheMissingPlan Plan =
                GetAssemblyOfTheMissingPlan();
            const echoes::sim::Entity* Oruun =
                Candidate->FindEntity(AssemblyOruunId);
            const echoes::sim::Entity* Verifier =
                Candidate->FindEntity(AssemblyVerifierId);
            const echoes::sim::Entity* MeridianPublicRecord =
                Candidate->FindEntity(
                    AssemblyMeridianPublicRecordInterfaceId);
            const echoes::sim::Entity* KharuunPublicRecord =
                Candidate->FindEntity(
                    AssemblyKharuunPublicRecordInterfaceId);
            const echoes::sim::Entity* CrownfallIndex =
                Candidate->FindEntity(
                    AssemblyCrownfallIndexInterfaceId);
            const auto IsKharuunScout = [](const echoes::sim::Entity* Entity)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::KharuunAssemblies &&
                       Entity->type == EntityType::ScoutUnit;
            };
            TSet<EntityId> CompositionIds;
            CompositionIds.Reserve(5);
            CompositionIds.Add(AssemblyOruunId);
            CompositionIds.Add(AssemblyVerifierId);
            CompositionIds.Add(
                AssemblyMeridianPublicRecordInterfaceId);
            CompositionIds.Add(
                AssemblyKharuunPublicRecordInterfaceId);
            CompositionIds.Add(AssemblyCrownfallIndexInterfaceId);
            if (!IsKharuunScout(Oruun) || !IsKharuunScout(Verifier) ||
                !IsPublicInterface(
                    MeridianPublicRecord,
                    Faction::MeridianCompact,
                    Plan.MeridianPublicRecordSite,
                    true) ||
                !IsPublicInterface(
                    KharuunPublicRecord,
                    Faction::KharuunAssemblies,
                    Plan.KharuunPublicRecordSite,
                    false) ||
                !IsPublicInterface(
                    CrownfallIndex,
                    Faction::MeridianCompact,
                    Plan.CrownfallIndexSite,
                    true) ||
                CompositionIds.Num() != 5)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active Assembly of the Missing composition");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            const echoes::sim::Entity* PossibleVoice =
                Candidate->FindEntity(SeveralVoicesPossibleVoiceId);
            const echoes::sim::Entity* ManifestVoice =
                Candidate->FindEntity(SeveralVoicesManifestVoiceId);
            const echoes::sim::Entity* Neme =
                Candidate->FindEntity(SeveralVoicesNemeId);
            const echoes::sim::Entity* ResearchLoom =
                Candidate->FindEntity(SeveralVoicesResearchLoomId);
            const auto IsChoirEntity = [](const echoes::sim::Entity* Entity,
                                          EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::HollowChoir &&
                       Entity->type == Type && Entity->hitPoints > 0;
            };
            TSet<EntityId> CompositionIds;
            CompositionIds.Reserve(4);
            CompositionIds.Add(SeveralVoicesPossibleVoiceId);
            CompositionIds.Add(SeveralVoicesManifestVoiceId);
            CompositionIds.Add(SeveralVoicesNemeId);
            CompositionIds.Add(SeveralVoicesResearchLoomId);
            if (!IsChoirEntity(PossibleVoice, EntityType::Soldier) ||
                !IsChoirEntity(ManifestVoice, EntityType::HeavyUnit) ||
                !IsChoirEntity(Neme, EntityType::ScoutUnit) ||
                !IsChoirEntity(ResearchLoom, EntityType::Barracks) ||
                PossibleVoice->choirIdentityState ==
                    echoes::sim::ChoirIdentityState::NotChoir ||
                ManifestVoice->choirIdentityState ==
                    echoes::sim::ChoirIdentityState::NotChoir ||
                Neme->choirIdentityState ==
                    echoes::sim::ChoirIdentityState::NotChoir ||
                CompositionIds.Num() != 4)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active Several Voices, One Command Choir composition");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignTheBrokenSun)
        {
            const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
            const echoes::sim::Vec2 ResolutionSite =
                FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                    Plan,
                    CandidateSelectedResolution);
            const echoes::sim::Entity* Voice =
                Candidate->FindEntity(BrokenSunAccordVoiceId);
            const echoes::sim::Entity* Heavy =
                Candidate->FindEntity(BrokenSunAccordHeavyId);
            const echoes::sim::Entity* Neme =
                Candidate->FindEntity(BrokenSunNemeId);
            const echoes::sim::Entity* Worker =
                Candidate->FindEntity(BrokenSunWorkerId);
            const echoes::sim::Entity* Mara =
                Candidate->FindEntity(BrokenSunMaraId);
            const echoes::sim::Entity* Oruun =
                Candidate->FindEntity(BrokenSunOruunId);
            const echoes::sim::Entity* Talar =
                Candidate->FindEntity(BrokenSunTalarId);
            const echoes::sim::Entity* Approach =
                Candidate->FindEntity(CandidateApproachAnchorId);
            const echoes::sim::Entity* Conduit =
                Candidate->FindEntity(CandidateResolutionConduitId);
            const auto HasIdentity = [](
                const echoes::sim::Entity* Entity,
                uint8 Owner,
                Faction EntityFaction,
                EntityType Type)
            {
                return Entity != nullptr && Entity->owner == Owner &&
                       Entity->faction == EntityFaction &&
                       Entity->type == Type;
            };
            const auto IsLiveIdentity = [&HasIdentity](
                const echoes::sim::Entity* Entity,
                uint8 Owner,
                Faction EntityFaction,
                EntityType Type)
            {
                return HasIdentity(Entity, Owner, EntityFaction, Type) &&
                       Entity->hitPoints > 0 && Entity->completed;
            };
            const bool bCommandComposition =
                IsLiveIdentity(
                    Voice,
                    LocalPlayerId,
                    Faction::HollowChoir,
                    EntityType::Soldier) &&
                IsLiveIdentity(
                    Heavy,
                    LocalPlayerId,
                    Faction::HollowChoir,
                    EntityType::HeavyUnit) &&
                IsLiveIdentity(
                    Neme,
                    LocalPlayerId,
                    Faction::HollowChoir,
                    EntityType::ScoutUnit) &&
                IsLiveIdentity(
                    Worker,
                    LocalPlayerId,
                    Faction::HollowChoir,
                    EntityType::Worker);
            const bool bWitnessComposition =
                IsLiveIdentity(
                    Mara,
                    2,
                    Faction::MeridianCompact,
                    EntityType::ScoutUnit) &&
                IsWithinTiles(
                    Mara->position,
                    Plan.MaraAccordSite,
                    BrokenSunSiteRadiusTiles) &&
                IsLiveIdentity(
                    Oruun,
                    3,
                    Faction::KharuunAssemblies,
                    EntityType::ScoutUnit) &&
                IsWithinTiles(
                    Oruun->position,
                    Plan.OruunAccordSite,
                    BrokenSunSiteRadiusTiles) &&
                IsLiveIdentity(
                    Talar,
                    2,
                    Faction::MeridianCompact,
                    EntityType::Worker) &&
                IsWithinTiles(
                    Talar->position,
                    Plan.TalarPublicRecordSite,
                    BrokenSunSiteRadiusTiles);
            const bool bApproachComposition =
                CandidateApproachAnchorId == 0 ||
                (IsLiveIdentity(
                     Approach,
                     LocalPlayerId,
                     Faction::HollowChoir,
                     EntityType::UtilityStructure) &&
                 IsWithinTiles(
                     Approach->position,
                     Plan.CrownfallApproachSite,
                     BrokenSunSiteRadiusTiles));
            const bool bConduitComposition =
                CandidateResolutionConduitId == 0 ||
                (IsLiveIdentity(
                     Conduit,
                     LocalPlayerId,
                     Faction::HollowChoir,
                     EntityType::UtilityStructure) &&
                 IsWithinTiles(
                     Conduit->position,
                     ResolutionSite,
                     BrokenSunConvergenceRadiusTiles));
            const echoes::sim::PlayerState* LocalPlayer =
                Candidate->FindPlayer(LocalPlayerId);
            const bool bSelectedAccord =
                CandidateSelectedResolution ==
                    EEchoesFinalResolution::None ||
                (Voice != nullptr && Heavy != nullptr && Neme != nullptr &&
                 Voice->choirIdentityState ==
                     echoes::sim::ChoirIdentityState::Possible &&
                 Heavy->choirIdentityState ==
                     echoes::sim::ChoirIdentityState::Manifest &&
                 Neme->choirIdentityState !=
                     echoes::sim::ChoirIdentityState::NotChoir &&
                 IsWithinTiles(
                     Voice->position,
                     Plan.MaraAccordSite,
                     BrokenSunSiteRadiusTiles) &&
                 IsWithinTiles(
                     Heavy->position,
                     Plan.OruunAccordSite,
                     BrokenSunSiteRadiusTiles) &&
                 IsWithinTiles(
                     Neme->position,
                     Plan.NemeAccordSite,
                     BrokenSunSiteRadiusTiles) &&
                 LocalPlayer != nullptr &&
                 LocalPlayer->HasCompletedResearch(
                     echoes::sim::ResearchType::ChoirHeldAlternatives) &&
                 LocalPlayer->HasCompletedResearch(
                     echoes::sim::ResearchType::ChoirSharedResolution) &&
                 CandidateApproachAnchorId != 0);
            TSet<EntityId> CompositionIds;
            CompositionIds.Reserve(9);
            CompositionIds.Add(BrokenSunAccordVoiceId);
            CompositionIds.Add(BrokenSunAccordHeavyId);
            CompositionIds.Add(BrokenSunNemeId);
            CompositionIds.Add(BrokenSunWorkerId);
            CompositionIds.Add(BrokenSunMaraId);
            CompositionIds.Add(BrokenSunOruunId);
            CompositionIds.Add(BrokenSunTalarId);
            int32 ExpectedCompositionCount = 7;
            if (CandidateApproachAnchorId != 0)
            {
                CompositionIds.Add(CandidateApproachAnchorId);
                ++ExpectedCompositionCount;
            }
            if (CandidateResolutionConduitId != 0)
            {
                CompositionIds.Add(CandidateResolutionConduitId);
                ++ExpectedCompositionCount;
            }
            const bool bLiveContractValid =
                bCommandComposition && bWitnessComposition &&
                bApproachComposition && bConduitComposition &&
                bSelectedAccord;
            if (CompositionIds.Num() != ExpectedCompositionCount ||
                (!bCandidateResolutionContractFailed &&
                 !bLiveContractValid) ||
                (bCandidateResolutionContractFailed &&
                 ((Approach != nullptr &&
                   !HasIdentity(
                       Approach,
                       LocalPlayerId,
                       Faction::HollowChoir,
                       EntityType::UtilityStructure)) ||
                  (Conduit != nullptr &&
                   !HasIdentity(
                       Conduit,
                       LocalPlayerId,
                       Faction::HollowChoir,
                       EntityType::UtilityStructure)))))
            {
                OutFailure = TEXT(
                    "snapshot does not match the active The Broken Sun protected composition or exact objective identities");
                return false;
            }
        }
        LoadedSimulation =
            MakeUnique<echoes::sim::Simulation>(std::move(*Candidate));
        bLoadedCrisisHoldStarted = bCandidateCrisisHoldStarted;
        bLoadedCrisisContractFailed = bCandidateCrisisContractFailed;
        LoadedPendingResolution = CandidatePendingResolution;
        LoadedSelectedResolution = CandidateSelectedResolution;
        bLoadedResolutionHoldStarted =
            bCandidateResolutionHoldStarted;
        bLoadedResolutionContractFailed =
            bCandidateResolutionContractFailed;
        LoadedResolutionStartTick = CandidateResolutionStartTick;
        LoadedApproachAnchorId = CandidateApproachAnchorId;
        LoadedResolutionConduitId = CandidateResolutionConduitId;
        return true;
    };

    if (TryLoad(SavePath, PrimaryFailure))
    {
        SelectedPath = SavePath;
    }
    else
    {
        FString StagedBackupFailure;
        if (TryLoad(BackupTemporaryPath, StagedBackupFailure))
        {
            SelectedPath = BackupTemporaryPath;
        }
        else
        {
            FString BackupFailure;
            if (!TryLoad(BackupPath, BackupFailure))
            {
                OutFeedback = FString::Printf(
                    TEXT("[LOAD_NO_VALID_CHECKPOINT] primary=%s; backup=%s; staged=%s"),
                    *PrimaryFailure,
                    *BackupFailure,
                    *StagedBackupFailure);
                return false;
            }
            SelectedPath = BackupPath;
        }
    }

    TUniquePtr<echoes::sim::Simulation> PreviousSimulation =
        MoveTemp(Simulation);
    DestroyEntityViews();
    DestroyFogView();
    DestroyTerrainView();
    Simulation = MoveTemp(LoadedSimulation);
    bScenarioReady = false;
    const bool bViewsRestored =
        SpawnTerrainView() && SpawnFogView() && SyncEntityViews(true);
    if (!bViewsRestored)
    {
        DestroyEntityViews();
        DestroyFogView();
        DestroyTerrainView();
        Simulation = MoveTemp(PreviousSimulation);
        const bool bRollbackViews =
            SpawnTerrainView() && SpawnFogView() && SyncEntityViews(true);
        bScenarioReady = bRollbackViews;
        OutFeedback = bRollbackViews
                          ? TEXT("[LOAD_VIEW_RESTORE_FAILED] The prior live match was restored.")
                          : TEXT("[LOAD_ROLLBACK_FAILED] Presentation recovery failed.");
        return false;
    }

    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence =
        *Simulation->NextCommandSequence(LocalPlayerId);
    bScenarioReady = true;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = true;
    bSimulationPaused = false;
    bMatchResultReported =
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing;
    bSeveralVoicesCrisisHoldStarted =
        SelectedOperation ==
                EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        bLoadedCrisisHoldStarted;
    bSeveralVoicesCrisisContractFailed =
        SelectedOperation ==
                EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        bLoadedCrisisContractFailed;
    if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun)
    {
        PendingBrokenSunResolution = LoadedPendingResolution;
        SelectedBrokenSunResolution = LoadedSelectedResolution;
        bBrokenSunResolutionHoldStarted =
            bLoadedResolutionHoldStarted;
        bBrokenSunResolutionContractFailed =
            bLoadedResolutionContractFailed;
        BrokenSunResolutionStartTick = LoadedResolutionStartTick;
        BrokenSunApproachAnchorId = LoadedApproachAnchorId;
        BrokenSunResolutionConduitId = LoadedResolutionConduitId;
    }
    const bool bUsedBackup = SelectedPath == BackupPath;
    const bool bUsedStagedBackup =
        SelectedPath == BackupTemporaryPath;
    OutFeedback = FString::Printf(
        TEXT("QUICK LOAD: tick %llu restored%s."),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        bUsedBackup
            ? TEXT(" from prior-generation backup")
        : bUsedStagedBackup
            ? TEXT(" from staged prior-generation recovery")
            : TEXT(""));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_QUICK_LOAD] tick=%llu source=%s ledgerBound=%s"),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        bUsedBackup
            ? TEXT("backup")
        : bUsedStagedBackup
            ? TEXT("staged_recovery")
            : TEXT("primary"),
        SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun ||
                RequiresCampaignBranchBoundQuickSave(SelectedOperation) ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignChoirAtLumeReach ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignNoNeutralLedger ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignFutureThatWon ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignAssemblyOfTheMissing ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand
            ? TEXT("true")
            : TEXT("false"));
    return true;
}

void UEchoesSimulationSubsystem::SetScenarioPaused(bool bPaused)
{
    if (!bScenarioReady || !Simulation.IsValid() ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        return;
    }
    if (bSustainedStressScenario)
    {
        if (bPaused)
        {
            FailSustainedStressContract(
                TEXT("PAUSE_REQUESTED"),
                TEXT("The sustained qualification fixture cannot be paused."));
        }
        return;
    }
    bSimulationPaused = bPaused;
    FixedTimeAccumulator = 0.0;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_MATCH_PAUSE] paused=%s"),
        bSimulationPaused ? TEXT("true") : TEXT("false"));
}

void UEchoesSimulationSubsystem::SetNetworkHumanOpponent(bool bEnabled)
{
    bNetworkHumanOpponent = bEnabled;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_COMMAND_POLICY] active=%s authorityDelayTicks=%u opponentAiSuppressed=%s"),
        bEnabled ? TEXT("true") : TEXT("false"),
        bEnabled ? 3U : 1U,
        bEnabled ? TEXT("true") : TEXT("false"));
}

echoes::sim::MatchOutcome UEchoesSimulationSubsystem::GetMatchOutcome() const
{
    return Simulation.IsValid() ? Simulation->Outcome()
                                : echoes::sim::MatchOutcome::Ongoing;
}

EEchoesProloguePhase UEchoesSimulationSubsystem::GetProloguePhase() const
{
    FEchoesPrologueMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesProloguePhase::Inactive;
    }

    const echoes::sim::Entity* Carrier = Simulation->FindEntity(ArchiveCarrierId);
    Facts.bArchiveCarrierIntact = Carrier != nullptr && Carrier->hitPoints > 0;
    if (Facts.bArchiveCarrierIntact)
    {
        Facts.bArchiveCarrierAtRecoverySite = IsWithinTiles(
            Carrier->position,
            GetArchiveRecoverySite(),
            PrologueSiteRadiusTiles);
        Facts.bArchiveCarrierAtEvacuationSite = IsWithinTiles(
            Carrier->position,
            GetEvacuationSite(),
            PrologueSiteRadiusTiles);
    }
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore &&
            Entity.hitPoints > 0)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.type == echoes::sim::EntityType::FutureWell &&
            Entity.wellChoice != echoes::sim::FutureWellChoice::Dormant)
        {
            Facts.bFutureWellProtocolChosen = Entity.owner == LocalPlayerId;
            Facts.bFutureWellLost = Entity.owner != LocalPlayerId;
        }
    }
    return FEchoesPrologueMissionModel::DeterminePhase(Facts);
}

bool UEchoesSimulationSubsystem::IsSevenAccountsUnlocked() const
{
    return bCampaignProgressAvailable &&
           CampaignProgress.FindDecision(
               EEchoesCampaignMissionId::WhatTheLedgerKeeps) != nullptr;
}

bool UEchoesSimulationSubsystem::IsCityReserveUnlocked() const
{
    if (!bCampaignProgressAvailable)
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* SevenAccounts =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::SevenAccountsOfRain);
    return Prologue != nullptr && SevenAccounts != nullptr &&
           Prologue->WellChoice == SevenAccounts->WellChoice;
}

bool UEchoesSimulationSubsystem::IsUnburiedRoadUnlocked() const
{
    if (!IsCityReserveUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* CityReserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ACityOnReserve);
    return Prologue != nullptr && CityReserve != nullptr &&
           Prologue->WellChoice == CityReserve->WellChoice;
}

bool UEchoesSimulationSubsystem::IsTermsOfContinuanceUnlocked() const
{
    if (!IsUnburiedRoadUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* UnburiedRoad =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TheUnburiedRoad);
    return Prologue != nullptr && UnburiedRoad != nullptr &&
           Prologue->WellChoice == UnburiedRoad->WellChoice;
}

bool UEchoesSimulationSubsystem::IsNamesWithoutBirthsUnlocked() const
{
    if (!IsTermsOfContinuanceUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Continuance =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TermsOfContinuance);
    return Prologue != nullptr && Continuance != nullptr &&
           Prologue->WellChoice == Continuance->WellChoice;
}

bool UEchoesSimulationSubsystem::IsShapeOfSilenceUnlocked() const
{
    if (!IsNamesWithoutBirthsUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Names =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::NamesWithoutBirths);
    return Prologue != nullptr && Names != nullptr &&
           Prologue->WellChoice == Names->WellChoice;
}

bool UEchoesSimulationSubsystem::IsShapeBesideUsUnlocked() const
{
    if (!IsShapeOfSilenceUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Shape =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TheShapeOfSilence);
    return Prologue != nullptr && Shape != nullptr &&
           Prologue->WellChoice == Shape->WellChoice;
}

bool UEchoesSimulationSubsystem::IsReserveAuthorityUnlocked() const
{
    if (!IsShapeBesideUsUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Shape =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TheShapeBesideUs);
    return Prologue != nullptr && Shape != nullptr &&
           Prologue->WellChoice == Shape->WellChoice;
}

bool UEchoesSimulationSubsystem::IsChoirAtLumeReachUnlocked() const
{
    if (!IsReserveAuthorityUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    return Prologue != nullptr && Reserve != nullptr &&
           Prologue->WellChoice == Reserve->WellChoice;
}

bool UEchoesSimulationSubsystem::IsNoNeutralLedgerUnlocked() const
{
    if (!IsChoirAtLumeReachUnlocked() ||
        CampaignProgress.Decisions.Num() < 10 ||
        CampaignProgress.Decisions.Num() > CampaignMissionCount)
    {
        return false;
    }
    for (int32 Index = 0; Index < 10; ++Index)
    {
        if (static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
            static_cast<uint8>(Index + 1))
        {
            return false;
        }
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Lume =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    if (Prologue == nullptr || Reserve == nullptr || Lume == nullptr)
    {
        return false;
    }
    for (int32 Index = 1; Index <= 8; ++Index)
    {
        if (CampaignProgress.Decisions[Index].WellChoice !=
            Prologue->WellChoice)
        {
            return false;
        }
    }
    FEchoesNoNeutralLedgerPlan Plan;
    return FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
        Prologue->WellChoice,
        Reserve->VerifiedFacts,
        Lume->WellChoice,
        Plan);
}

bool UEchoesSimulationSubsystem::IsFutureThatWonUnlocked() const
{
    if (CampaignProgress.Decisions.Num() < 11 ||
        CampaignProgress.Decisions.Num() > CampaignMissionCount)
    {
        return false;
    }
    for (int32 Index = 0; Index < 11; ++Index)
    {
        if (static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
            static_cast<uint8>(Index + 1))
        {
            return false;
        }
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Lume =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    const FEchoesCampaignDecisionRecord* Alliance =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::NoNeutralLedger);
    if (Prologue == nullptr || Reserve == nullptr || Lume == nullptr ||
        Alliance == nullptr || Alliance->WellChoice != Lume->WellChoice)
    {
        return false;
    }
    for (int32 Index = 1; Index <= 8; ++Index)
    {
        if (CampaignProgress.Decisions[Index].WellChoice !=
            Prologue->WellChoice)
        {
            return false;
        }
    }
    FEchoesFutureThatWonPlan Plan;
    return FEchoesFutureThatWonMissionModel::TryPlanForLedger(
        Prologue->WellChoice,
        Reserve->VerifiedFacts,
        Alliance->WellChoice,
        Plan);
}

bool UEchoesSimulationSubsystem::IsAssemblyOfTheMissingUnlocked() const
{
    if (!bCampaignProgressAvailable ||
        CampaignProgress.Decisions.Num() < 12 ||
        CampaignProgress.Decisions.Num() > CampaignMissionCount)
    {
        return false;
    }
    for (int32 Index = 0; Index < 12; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            return false;
        }
    }

    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Lume =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    const FEchoesCampaignDecisionRecord* Alliance =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::NoNeutralLedger);
    const FEchoesCampaignDecisionRecord* Restoration =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TheFutureThatWon);
    if (Prologue == nullptr || Reserve == nullptr || Lume == nullptr ||
        Alliance == nullptr || Restoration == nullptr ||
        Alliance->WellChoice != Lume->WellChoice ||
        Restoration->WellChoice != Lume->WellChoice ||
        Restoration->SimulationSnapshotVersion < 21 ||
        Restoration->SimulationSnapshotVersion >
            echoes::sim::kSnapshotVersion)
    {
        return false;
    }
    for (int32 Index = 1; Index <= 8; ++Index)
    {
        if (CampaignProgress.Decisions[Index].WellChoice !=
            Prologue->WellChoice)
        {
            return false;
        }
    }
    FEchoesAssemblyOfTheMissingPlan Plan;
    return FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
        Prologue->WellChoice,
        Reserve->VerifiedFacts,
        Restoration->WellChoice,
        Plan);
}

bool UEchoesSimulationSubsystem::IsSeveralVoicesOneCommandUnlocked() const
{
    if (!bCampaignProgressAvailable ||
        CampaignProgress.Decisions.Num() < 13 ||
        CampaignProgress.Decisions.Num() > CampaignMissionCount)
    {
        return false;
    }
    for (int32 Index = 0; Index < 13; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            return false;
        }
    }

    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Lume =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    const FEchoesCampaignDecisionRecord* Alliance =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::NoNeutralLedger);
    const FEchoesCampaignDecisionRecord* Restoration =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TheFutureThatWon);
    const FEchoesCampaignDecisionRecord* Assembly =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::AssemblyOfTheMissing);
    if (Prologue == nullptr || Reserve == nullptr || Lume == nullptr ||
        Alliance == nullptr || Restoration == nullptr ||
        Assembly == nullptr || Alliance->WellChoice != Lume->WellChoice ||
        Restoration->WellChoice != Lume->WellChoice ||
        Assembly->WellChoice != Lume->WellChoice ||
        Assembly->SimulationSnapshotVersion < 21 ||
        Assembly->SimulationSnapshotVersion > echoes::sim::kSnapshotVersion)
    {
        return false;
    }
    for (int32 Index = 1; Index <= 8; ++Index)
    {
        if (CampaignProgress.Decisions[Index].WellChoice !=
            Prologue->WellChoice)
        {
            return false;
        }
    }

    FEchoesSeveralVoicesOneCommandPlan Plan;
    return FEchoesSeveralVoicesOneCommandMissionModel::TryPlanForLedger(
        Prologue->WellChoice,
        Reserve->VerifiedFacts,
        Assembly->WellChoice,
        Plan);
}

bool UEchoesSimulationSubsystem::IsBrokenSunUnlocked() const
{
    if (!bCampaignProgressAvailable ||
        CampaignProgress.Decisions.Num() < 14 ||
        CampaignProgress.Decisions.Num() > CampaignMissionCount)
    {
        return false;
    }
    for (int32 Index = 0; Index < 14; ++Index)
    {
        if (!CampaignProgress.Decisions.IsValidIndex(Index) ||
            static_cast<uint8>(CampaignProgress.Decisions[Index].Mission) !=
                static_cast<uint8>(Index + 1))
        {
            return false;
        }
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Lume =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    const FEchoesCampaignDecisionRecord* Voices =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::SeveralVoicesOneCommand);
    if (Prologue == nullptr || Reserve == nullptr || Lume == nullptr ||
        Voices == nullptr || Voices->WellChoice != Lume->WellChoice ||
        Voices->SimulationSnapshotVersion < 22 ||
        Voices->SimulationSnapshotVersion > echoes::sim::kSnapshotVersion)
    {
        return false;
    }
    for (int32 Index = 1; Index <= 8; ++Index)
    {
        if (CampaignProgress.Decisions[Index].WellChoice !=
            Prologue->WellChoice)
        {
            return false;
        }
    }
    FEchoesBrokenSunPlan Plan;
    return FEchoesBrokenSunMissionModel::TryPlanForLedger(
        Prologue->WellChoice,
        Reserve->VerifiedFacts,
        Voices->WellChoice,
        Plan);
}

FutureWellChoice UEchoesSimulationSubsystem::GetRecordedPrologueChoice() const
{
    const FEchoesCampaignDecisionRecord* Record =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    return Record != nullptr ? Record->WellChoice : FutureWellChoice::Dormant;
}

FEchoesSevenAccountsRoute
UEchoesSimulationSubsystem::GetSevenAccountsRoute() const
{
    return FEchoesSevenAccountsMissionModel::RouteForChoice(
        GetRecordedPrologueChoice());
}

FEchoesCityReserveGrid UEchoesSimulationSubsystem::GetCityReserveGrid() const
{
    return FEchoesCityReserveMissionModel::GridForChoice(
        GetRecordedPrologueChoice());
}

FEchoesUnburiedRoadRoute
UEchoesSimulationSubsystem::GetUnburiedRoadRoute() const
{
    return FEchoesUnburiedRoadMissionModel::RouteForChoice(
        GetRecordedPrologueChoice());
}

FEchoesTermsOfContinuancePlan
UEchoesSimulationSubsystem::GetTermsOfContinuancePlan() const
{
    return FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

FEchoesNamesWithoutBirthsPlan
UEchoesSimulationSubsystem::GetNamesWithoutBirthsPlan() const
{
    return FEchoesNamesWithoutBirthsMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

FEchoesShapeOfSilencePlan
UEchoesSimulationSubsystem::GetShapeOfSilencePlan() const
{
    return FEchoesShapeOfSilenceMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

FEchoesShapeBesideUsPlan
UEchoesSimulationSubsystem::GetShapeBesideUsPlan() const
{
    return FEchoesShapeBesideUsMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

FEchoesReserveAuthorityPlan
UEchoesSimulationSubsystem::GetReserveAuthorityPlan() const
{
    return FEchoesReserveAuthorityMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

EEchoesCityDistrict
UEchoesSimulationSubsystem::GetReserveAuthorityDeferredDistrict() const
{
    FEchoesReserveAuthorityMissionFacts Facts;
    if (!Simulation.IsValid())
    {
        return EEchoesCityDistrict::LifeSupport;
    }
    const echoes::sim::Entity* LifeSupport =
        Simulation->FindEntity(LifeSupportDistrictId);
    const echoes::sim::Entity* Transit =
        Simulation->FindEntity(TransitDistrictId);
    const echoes::sim::Entity* Archive =
        Simulation->FindEntity(ArchiveDistrictId);
    Facts.bLifeSupportPowered =
        LifeSupport != nullptr && LifeSupport->hitPoints > 0 &&
        LifeSupport->aegisPowered;
    Facts.bTransitPowered =
        Transit != nullptr && Transit->hitPoints > 0 && Transit->aegisPowered;
    Facts.bArchivePowered =
        Archive != nullptr && Archive->hitPoints > 0 && Archive->aegisPowered;
    return FEchoesReserveAuthorityMissionModel::DeferredDistrict(Facts);
}

FEchoesChoirAtLumeReachPlan
UEchoesSimulationSubsystem::GetChoirAtLumeReachPlan() const
{
    EEchoesCityDistrict DeferredDistrict = EEchoesCityDistrict::LifeSupport;
    if (const FEchoesCampaignDecisionRecord* Reserve =
            CampaignProgress.FindDecision(
                EEchoesCampaignMissionId::ReserveAuthority))
    {
        const uint8 Facts = Reserve->VerifiedFacts;
        DeferredDistrict =
            (Facts & static_cast<uint8>(
                 EEchoesReserveAuthorityCompletionFact::LifeSupportPowered)) == 0
                ? EEchoesCityDistrict::LifeSupport
            : (Facts & static_cast<uint8>(
                   EEchoesReserveAuthorityCompletionFact::TransitPowered)) == 0
                ? EEchoesCityDistrict::Transit
                : EEchoesCityDistrict::Archive;
    }
    return FEchoesChoirAtLumeReachMissionModel::PlanForChoice(
        GetRecordedPrologueChoice(), DeferredDistrict);
}

FEchoesNoNeutralLedgerPlan
UEchoesSimulationSubsystem::GetNoNeutralLedgerPlan() const
{
    FEchoesNoNeutralLedgerPlan Plan;
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Lume =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    if (Prologue == nullptr || Reserve == nullptr || Lume == nullptr ||
        !FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
            Prologue->WellChoice,
            Reserve->VerifiedFacts,
            Lume->WellChoice,
            Plan))
    {
        return {};
    }
    return Plan;
}

FEchoesFutureThatWonPlan
UEchoesSimulationSubsystem::GetFutureThatWonPlan() const
{
    FEchoesFutureThatWonPlan Plan;
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Alliance =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::NoNeutralLedger);
    if (Prologue == nullptr || Reserve == nullptr || Alliance == nullptr ||
        !FEchoesFutureThatWonMissionModel::TryPlanForLedger(
            Prologue->WellChoice,
            Reserve->VerifiedFacts,
            Alliance->WellChoice,
            Plan))
    {
        return {};
    }
    return Plan;
}

FEchoesAssemblyOfTheMissingPlan
UEchoesSimulationSubsystem::GetAssemblyOfTheMissingPlan() const
{
    FEchoesAssemblyOfTheMissingPlan Plan;
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Restoration =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TheFutureThatWon);
    if (Prologue == nullptr || Reserve == nullptr || Restoration == nullptr ||
        !FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
            Prologue->WellChoice,
            Reserve->VerifiedFacts,
            Restoration->WellChoice,
            Plan))
    {
        return {};
    }
    return Plan;
}

FEchoesSeveralVoicesOneCommandPlan
UEchoesSimulationSubsystem::GetSeveralVoicesOneCommandPlan() const
{
    FEchoesSeveralVoicesOneCommandPlan Plan;
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Assembly =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::AssemblyOfTheMissing);
    if (Prologue == nullptr || Reserve == nullptr || Assembly == nullptr ||
        !FEchoesSeveralVoicesOneCommandMissionModel::TryPlanForLedger(
            Prologue->WellChoice,
            Reserve->VerifiedFacts,
            Assembly->WellChoice,
            Plan))
    {
        return {};
    }
    return Plan;
}

FEchoesBrokenSunPlan UEchoesSimulationSubsystem::GetBrokenSunPlan() const
{
    FEchoesBrokenSunPlan Plan;
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Reserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    const FEchoesCampaignDecisionRecord* Voices =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::SeveralVoicesOneCommand);
    if (Prologue == nullptr || Reserve == nullptr || Voices == nullptr ||
        !FEchoesBrokenSunMissionModel::TryPlanForLedger(
            Prologue->WellChoice,
            Reserve->VerifiedFacts,
            Voices->WellChoice,
            Plan))
    {
        return {};
    }
    return Plan;
}

echoes::sim::EntityId UEchoesSimulationSubsystem::GetCityDistrictId(
    EEchoesCityDistrict District) const
{
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return LifeSupportDistrictId;
        case EEchoesCityDistrict::Transit:
            return TransitDistrictId;
        case EEchoesCityDistrict::Archive:
            return ArchiveDistrictId;
    }
    return 0;
}

EEchoesSevenAccountsPhase
UEchoesSimulationSubsystem::GetSevenAccountsPhase() const
{
    FEchoesSevenAccountsMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesSevenAccountsPhase::Inactive;
    }

    const FEchoesSevenAccountsRoute Route = GetSevenAccountsRoute();
    const echoes::sim::Entity* Bearer =
        Simulation->FindEntity(MemoryBearerId);
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(MigrationWaystoneId);
    Facts.bMemoryBearerIntact = Bearer != nullptr && Bearer->hitPoints > 0;
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bMemoryBearerAtAccountSite =
        Facts.bMemoryBearerIntact &&
        IsWithinTiles(
            Bearer->position,
            Route.MemoryAccountSite,
            SevenAccountsSiteRadiusTiles);
    Facts.bWaystoneRootedAtAnchor =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Route.WaystoneAnchor,
            SevenAccountsSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.type == EntityType::CommandCore && Entity.hitPoints > 0)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesSevenAccountsMissionModel::DeterminePhase(Facts);
}

EEchoesCityReservePhase UEchoesSimulationSubsystem::GetCityReservePhase() const
{
    FEchoesCityReserveMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignCityReserve &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesCityReservePhase::Inactive;
    }

    const echoes::sim::Entity* LifeSupport =
        Simulation->FindEntity(LifeSupportDistrictId);
    const echoes::sim::Entity* Transit =
        Simulation->FindEntity(TransitDistrictId);
    const echoes::sim::Entity* Archive =
        Simulation->FindEntity(ArchiveDistrictId);
    Facts.bLifeSupportIntact =
        LifeSupport != nullptr && LifeSupport->hitPoints > 0;
    Facts.bTransitIntact = Transit != nullptr && Transit->hitPoints > 0;
    Facts.bArchiveIntact = Archive != nullptr && Archive->hitPoints > 0;
    Facts.bLifeSupportPowered =
        Facts.bLifeSupportIntact && LifeSupport->aegisPowered;
    Facts.bTransitPowered =
        Facts.bTransitIntact && Transit->aegisPowered;
    Facts.bArchivePowered =
        Facts.bArchiveIntact && Archive->aegisPowered;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.type == EntityType::CommandCore && Entity.hitPoints > 0)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesCityReserveMissionModel::DeterminePhase(
        Facts,
        GetCityReserveGrid());
}

EEchoesUnburiedRoadPhase
UEchoesSimulationSubsystem::GetUnburiedRoadPhase() const
{
    FEchoesUnburiedRoadMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesUnburiedRoadPhase::Inactive;
    }

    const FEchoesUnburiedRoadRoute Route = GetUnburiedRoadRoute();
    const echoes::sim::Entity* Bearer =
        Simulation->FindEntity(MemoryBearerId);
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(MigrationWaystoneId);
    Facts.bMemoryBearerIntact = Bearer != nullptr && Bearer->hitPoints > 0;
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bMemoryBearerAtShard =
        Facts.bMemoryBearerIntact &&
        IsWithinTiles(
            Bearer->position,
            Route.MemoryShardSite,
            UnburiedRoadSiteRadiusTiles);
    Facts.bWaystoneRootedAtRoadhead =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Route.Roadhead,
            UnburiedRoadSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                Route.ListeningSpineSite,
                UnburiedRoadSiteRadiusTiles))
        {
            Facts.bListeningSpineComplete = true;
        }
    }
    return FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts);
}

EEchoesTermsOfContinuancePhase
UEchoesSimulationSubsystem::GetTermsOfContinuancePhase() const
{
    FEchoesTermsOfContinuanceMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesTermsOfContinuancePhase::Inactive;
    }

    const FEchoesTermsOfContinuancePlan Plan =
        GetTermsOfContinuancePlan();
    const echoes::sim::Entity* MeridianRelay =
        Simulation->FindEntity(MeridianContinuanceRelayId);
    const echoes::sim::Entity* KharuunSpine =
        Simulation->FindEntity(KharuunContinuanceSpineId);
    const echoes::sim::Entity* MeridianWitness =
        Simulation->FindEntity(MeridianContinuanceWitnessId);
    const echoes::sim::Entity* KharuunWitness =
        Simulation->FindEntity(KharuunContinuanceWitnessId);
    Facts.bMeridianRelayIntact =
        MeridianRelay != nullptr && MeridianRelay->hitPoints > 0;
    Facts.bKharuunSpineIntact =
        KharuunSpine != nullptr && KharuunSpine->hitPoints > 0;
    Facts.bMeridianWitnessIntact =
        MeridianWitness != nullptr && MeridianWitness->hitPoints > 0;
    Facts.bKharuunWitnessIntact =
        KharuunWitness != nullptr && KharuunWitness->hitPoints > 0;
    Facts.bMeridianRelaySynchronized =
        Facts.bMeridianRelayIntact && MeridianRelay->aegisPowered;
    Facts.bKharuunSpineSynchronized =
        Facts.bKharuunSpineIntact && KharuunSpine->aegisPowered;
    const uint64 CurrentTick = Simulation->CurrentTick();
    Facts.bContinuanceWindowHeld =
        CurrentTick >= Plan.ContinuanceWindowEndTick;
    Facts.bContinuanceWindowCompromised =
        CurrentTick >= Plan.ContinuanceWindowStartTick &&
        (!Facts.bMeridianRelaySynchronized ||
         !Facts.bKharuunSpineSynchronized);
    const bool bMeridianWitnessAtExtraction =
        Facts.bMeridianWitnessIntact &&
        IsWithinTiles(
            MeridianWitness->position,
            Plan.WitnessExtractionSite,
            TermsOfContinuanceSiteRadiusTiles);
    const bool bKharuunWitnessAtExtraction =
        Facts.bKharuunWitnessIntact &&
        IsWithinTiles(
            KharuunWitness->position,
            Plan.WitnessExtractionSite,
            TermsOfContinuanceSiteRadiusTiles);
    const auto IsEarlyExtractionOrder = [&Plan](
                                            const echoes::sim::Entity* Witness)
    {
        return Witness != nullptr &&
               Witness->order.type == echoes::sim::OrderType::Move &&
               IsWithinTiles(
                   Witness->order.destination,
                   Plan.WitnessExtractionSite,
                   TermsOfContinuanceSiteRadiusTiles);
    };
    Facts.bWitnessExtractionStartedEarly =
        CurrentTick < Plan.ContinuanceWindowEndTick &&
        (bMeridianWitnessAtExtraction || bKharuunWitnessAtExtraction ||
         IsEarlyExtractionOrder(MeridianWitness) ||
         IsEarlyExtractionOrder(KharuunWitness));
    Facts.bMeridianWitnessExtracted =
        Facts.bContinuanceWindowHeld && bMeridianWitnessAtExtraction;
    Facts.bKharuunWitnessExtracted =
        Facts.bContinuanceWindowHeld && bKharuunWitnessAtExtraction;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts);
}

EEchoesNamesWithoutBirthsPhase
UEchoesSimulationSubsystem::GetNamesWithoutBirthsPhase() const
{
    FEchoesNamesWithoutBirthsMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesNamesWithoutBirthsPhase::Inactive;
    }

    const FEchoesNamesWithoutBirthsPlan Plan =
        GetNamesWithoutBirthsPlan();
    const echoes::sim::Entity* Talar = Simulation->FindEntity(TalarId);
    const echoes::sim::Entity* Archive =
        Simulation->FindEntity(CensusArchiveId);
    const echoes::sim::Entity* FirstCivilian =
        Simulation->FindEntity(FirstCivilianId);
    const echoes::sim::Entity* SecondCivilian =
        Simulation->FindEntity(SecondCivilianId);
    Facts.bTalarIntact = Talar != nullptr && Talar->hitPoints > 0;
    Facts.bArchiveIntact = Archive != nullptr && Archive->hitPoints > 0;
    Facts.bFirstCivilianIntact =
        FirstCivilian != nullptr && FirstCivilian->hitPoints > 0;
    Facts.bSecondCivilianIntact =
        SecondCivilian != nullptr && SecondCivilian->hitPoints > 0;
    Facts.bArchivePowered =
        Facts.bArchiveIntact && Archive->aegisPowered;
    Facts.bCensusEvidenceLocated =
        Facts.bArchivePowered ||
        (Facts.bTalarIntact &&
         IsWithinTiles(
             Talar->position,
             Plan.CensusSite,
             NamesWithoutBirthsSiteRadiusTiles));
    Facts.bFirstCivilianSheltered =
        Facts.bFirstCivilianIntact &&
        IsWithinTiles(
            FirstCivilian->position,
            Plan.CivilianShelterSite,
            NamesWithoutBirthsSiteRadiusTiles);
    Facts.bSecondCivilianSheltered =
        Facts.bSecondCivilianIntact &&
        IsWithinTiles(
            SecondCivilian->position,
            Plan.CivilianShelterSite,
            NamesWithoutBirthsSiteRadiusTiles);
    Facts.bTalarAtEvidenceExtraction =
        Facts.bTalarIntact && Facts.bArchivePowered &&
        Facts.bFirstCivilianSheltered && Facts.bSecondCivilianSheltered &&
        IsWithinTiles(
            Talar->position,
            Plan.EvidenceExtractionSite,
            NamesWithoutBirthsSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts);
}

EEchoesShapeOfSilencePhase
UEchoesSimulationSubsystem::GetShapeOfSilencePhase() const
{
    FEchoesShapeOfSilenceMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesShapeOfSilencePhase::Inactive;
    }

    const FEchoesShapeOfSilencePlan Plan = GetShapeOfSilencePlan();
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(MigrationWaystoneId);
    const echoes::sim::Entity* Oruun = Simulation->FindEntity(OruunId);
    const echoes::sim::Entity* FirstWitness =
        Simulation->FindEntity(FirstMemoryWitnessId);
    const echoes::sim::Entity* SecondWitness =
        Simulation->FindEntity(SecondMemoryWitnessId);
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bOruunIntact = Oruun != nullptr && Oruun->hitPoints > 0;
    Facts.bFirstWitnessIntact =
        FirstWitness != nullptr && FirstWitness->hitPoints > 0;
    Facts.bSecondWitnessIntact =
        SecondWitness != nullptr && SecondWitness->hitPoints > 0;
    Facts.bWaystoneRootedAtAnchor =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Plan.WaystoneAnchor,
            ShapeOfSilenceSiteRadiusTiles);
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                Plan.ListeningSpineSite,
                ShapeOfSilenceSiteRadiusTiles))
        {
            Facts.bListeningSpineRaised = true;
        }
    }
    Facts.bFirstWitnessPositioned =
        Facts.bFirstWitnessIntact &&
        IsWithinTiles(
            FirstWitness->position,
            Plan.FirstWitnessSite,
            ShapeOfSilenceSiteRadiusTiles);
    Facts.bSecondWitnessPositioned =
        Facts.bSecondWitnessIntact &&
        IsWithinTiles(
            SecondWitness->position,
            Plan.SecondWitnessSite,
            ShapeOfSilenceSiteRadiusTiles);
    Facts.bOruunAtConfluence =
        Facts.bOruunIntact && Facts.bFirstWitnessPositioned &&
        Facts.bSecondWitnessPositioned &&
        IsWithinTiles(
            Oruun->position,
            Plan.ConfluenceSite,
            ShapeOfSilenceSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts);
}

EEchoesShapeBesideUsPhase
UEchoesSimulationSubsystem::GetShapeBesideUsPhase() const
{
    FEchoesShapeBesideUsMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignShapeBesideUs &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesShapeBesideUsPhase::Inactive;
    }

    const FEchoesShapeBesideUsPlan Plan = GetShapeBesideUsPlan();
    const echoes::sim::Entity* Talar =
        Simulation->FindEntity(ShapeBesideUsTalarId);
    const echoes::sim::Entity* FirstWitness =
        Simulation->FindEntity(FirstStateWitnessId);
    const echoes::sim::Entity* SecondWitness =
        Simulation->FindEntity(SecondStateWitnessId);
    Facts.bTalarIntact = Talar != nullptr && Talar->hitPoints > 0;
    Facts.bFirstStateWitnessIntact =
        FirstWitness != nullptr && FirstWitness->hitPoints > 0;
    Facts.bSecondStateWitnessIntact =
        SecondWitness != nullptr && SecondWitness->hitPoints > 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                Plan.EchoRelaySite,
                ShapeBesideUsSiteRadiusTiles))
        {
            Facts.bEchoRelayRaised = true;
        }
    }
    Facts.bFirstEchoObserved =
        Facts.bEchoRelayRaised ||
        (Facts.bTalarIntact &&
         IsWithinTiles(
             Talar->position,
             Plan.FirstEchoSite,
             ShapeBesideUsSiteRadiusTiles));
    Facts.bFirstStateTraversed =
        Facts.bFirstStateWitnessIntact &&
        IsWithinTiles(
            FirstWitness->position,
            Plan.FirstStateSite,
            ShapeBesideUsSiteRadiusTiles);
    Facts.bSecondStateTraversed =
        Facts.bSecondStateWitnessIntact &&
        IsWithinTiles(
            SecondWitness->position,
            Plan.SecondStateSite,
            ShapeBesideUsSiteRadiusTiles);
    Facts.bTalarAtConvergence =
        Facts.bTalarIntact && Facts.bFirstStateTraversed &&
        Facts.bSecondStateTraversed &&
        IsWithinTiles(
            Talar->position,
            Plan.ConvergenceSite,
            ShapeBesideUsSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts);
}

EEchoesReserveAuthorityPhase
UEchoesSimulationSubsystem::GetReserveAuthorityPhase() const
{
    FEchoesReserveAuthorityMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesReserveAuthorityPhase::Inactive;
    }

    const FEchoesReserveAuthorityPlan Plan = GetReserveAuthorityPlan();
    const echoes::sim::Entity* Mara =
        Simulation->FindEntity(ReserveAuthorityMaraId);
    const echoes::sim::Entity* LifeSupport =
        Simulation->FindEntity(LifeSupportDistrictId);
    const echoes::sim::Entity* Transit =
        Simulation->FindEntity(TransitDistrictId);
    const echoes::sim::Entity* Archive =
        Simulation->FindEntity(ArchiveDistrictId);
    Facts.bMaraIntact = Mara != nullptr && Mara->hitPoints > 0;
    Facts.bLifeSupportIntact =
        LifeSupport != nullptr && LifeSupport->hitPoints > 0;
    Facts.bTransitIntact = Transit != nullptr && Transit->hitPoints > 0;
    Facts.bArchiveIntact = Archive != nullptr && Archive->hitPoints > 0;
    Facts.bLifeSupportPowered =
        Facts.bLifeSupportIntact && LifeSupport->aegisPowered;
    Facts.bTransitPowered = Facts.bTransitIntact && Transit->aegisPowered;
    Facts.bArchivePowered = Facts.bArchiveIntact && Archive->aegisPowered;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    const int32 PoweredCount =
        FEchoesReserveAuthorityMissionModel::PoweredDistrictCount(Facts);
    Facts.bAuthoritySecured =
        PoweredCount > 0 ||
        (Facts.bMaraIntact &&
         IsWithinTiles(
             Mara->position,
             Plan.AuthoritySite,
             ReserveAuthoritySiteRadiusTiles));
    if (PoweredCount == 2 && Facts.bMaraIntact)
    {
        const EEchoesCityDistrict Deferred =
            FEchoesReserveAuthorityMissionModel::DeferredDistrict(Facts);
        Facts.bMaraAtDeferredDistrict = IsWithinTiles(
            Mara->position,
            FEchoesCityReserveMissionModel::SiteForDistrict(Deferred),
            ReserveAuthoritySiteRadiusTiles);
    }
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts);
}

EEchoesChoirAtLumeReachPhase
UEchoesSimulationSubsystem::GetChoirAtLumeReachPhase() const
{
    FEchoesChoirAtLumeReachMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignChoirAtLumeReach &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesChoirAtLumeReachPhase::Inactive;
    }

    const FEchoesChoirAtLumeReachPlan Plan = GetChoirAtLumeReachPlan();
    const echoes::sim::Entity* Oruun =
        Simulation->FindEntity(ChoirAtLumeReachOruunId);
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(ChoirAtLumeReachWaystoneId);
    const echoes::sim::Entity* Well =
        Simulation->FindEntity(ChoirAtLumeReachWellId);
    Facts.bOruunIntact = Oruun != nullptr && Oruun->hitPoints > 0;
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bFutureWellIntact = Well != nullptr && Well->hitPoints > 0 &&
        Well->type == EntityType::FutureWell;
    Facts.bDeferredLiabilityResolved =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Plan.LiabilitySite,
            ChoirAtLumeReachSiteRadiusTiles);

    EntityId FirstAnchorId = 0;
    EntityId SecondAnchorId = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure)
        {
            if (FirstAnchorId == 0 &&
                IsWithinTiles(
                    Entity.position,
                    Plan.FirstAnchorSite,
                    ChoirAtLumeReachSiteRadiusTiles))
            {
                FirstAnchorId = Entity.id;
            }
            if (SecondAnchorId == 0 && Entity.id != FirstAnchorId &&
                IsWithinTiles(
                    Entity.position,
                    Plan.SecondAnchorSite,
                    ChoirAtLumeReachSiteRadiusTiles))
            {
                SecondAnchorId = Entity.id;
            }
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
    }
    Facts.bFirstAnchorRaised = FirstAnchorId != 0;
    Facts.bSecondAnchorRaised = SecondAnchorId != 0;
    Facts.bFutureWellProtocolChosen =
        Facts.bFutureWellIntact &&
        Well->owner == LocalPlayerId &&
        Well->wellChoice != FutureWellChoice::Dormant;
    Facts.bBranchResolutionCompleted =
        Facts.bFutureWellProtocolChosen && Facts.bOruunIntact &&
        IsWithinTiles(
            Oruun->position,
            FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
                Well->wellChoice),
            ChoirAtLumeReachSiteRadiusTiles);
    Facts.bReshapeWindowExpired =
        Facts.bFutureWellProtocolChosen &&
        Well->wellChoice == FutureWellChoice::Reshape &&
        Well->reshapeUntilTick == 0 &&
        !Facts.bBranchResolutionCompleted;
    Facts.bContactEstablished =
        (Facts.bOruunIntact &&
         IsWithinTiles(
             Oruun->position,
             Plan.ContactSite,
             ChoirAtLumeReachSiteRadiusTiles)) ||
        Facts.bDeferredLiabilityResolved || Facts.bFirstAnchorRaised ||
        Facts.bSecondAnchorRaised || Facts.bFutureWellProtocolChosen;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts);
}

EEchoesNoNeutralLedgerPhase
UEchoesSimulationSubsystem::GetNoNeutralLedgerPhase() const
{
    FEchoesNoNeutralLedgerMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignNoNeutralLedger &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesNoNeutralLedgerPhase::Inactive;
    }

    const FEchoesNoNeutralLedgerPlan Plan = GetNoNeutralLedgerPlan();
    const echoes::sim::Entity* Oruun =
        Simulation->FindEntity(NoNeutralOruunId);
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(NoNeutralWaystoneId);
    const echoes::sim::Entity* Witness =
        Simulation->FindEntity(NoNeutralLedgerWitnessId);
    const echoes::sim::Entity* FirstDistrictInterface =
        Simulation->FindEntity(NoNeutralFirstDistrictInterfaceId);
    const echoes::sim::Entity* SecondDistrictInterface =
        Simulation->FindEntity(NoNeutralSecondDistrictInterfaceId);
    const echoes::sim::Entity* MeridianEvidenceInterface =
        Simulation->FindEntity(NoNeutralMeridianEvidenceInterfaceId);
    const echoes::sim::Entity* KharuunEvidenceInterface =
        Simulation->FindEntity(NoNeutralKharuunEvidenceInterfaceId);
    const echoes::sim::Entity* Well =
        Simulation->FindEntity(NoNeutralWellId);
    Facts.bOruunIntact = Oruun != nullptr && Oruun->hitPoints > 0;
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bLedgerWitnessIntact =
        Witness != nullptr && Witness->hitPoints > 0;
    Facts.bFutureWellIntact =
        Well != nullptr && Well->hitPoints > 0 &&
        Well->type == EntityType::FutureWell;
    const bool bFirstPublicDistrictAvailable = IsPublicInterface(
        FirstDistrictInterface,
        Faction::MeridianCompact,
        Plan.FirstDistrictSite,
        true);
    const bool bSecondPublicDistrictAvailable = IsPublicInterface(
        SecondDistrictInterface,
        Faction::MeridianCompact,
        Plan.SecondDistrictSite,
        true);
    const bool bMeridianEvidenceAvailable = IsPublicInterface(
        MeridianEvidenceInterface,
        Faction::MeridianCompact,
        Plan.MeridianEvidenceSite,
        true);
    const bool bKharuunEvidenceAvailable = IsPublicInterface(
        KharuunEvidenceInterface,
        Faction::KharuunAssemblies,
        Plan.KharuunEvidenceSite,
        false);
    Facts.bPublicInterfacesIntact =
        bFirstPublicDistrictAvailable && bSecondPublicDistrictAvailable &&
        bMeridianEvidenceAvailable && bKharuunEvidenceAvailable;
    Facts.bInheritedRouteSecured =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Plan.RouteSite,
            NoNeutralLedgerSiteRadiusTiles);

    EntityId FirstDistrictLink = 0;
    EntityId SecondDistrictLink = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure)
        {
            if (FirstDistrictLink == 0 &&
                IsWithinTiles(
                    Entity.position,
                    Plan.FirstDistrictSite,
                    NoNeutralLedgerSiteRadiusTiles))
            {
                FirstDistrictLink = Entity.id;
            }
            if (SecondDistrictLink == 0 &&
                Entity.id != FirstDistrictLink &&
                IsWithinTiles(
                    Entity.position,
                    Plan.SecondDistrictSite,
                    NoNeutralLedgerSiteRadiusTiles))
            {
                SecondDistrictLink = Entity.id;
            }
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
    }
    Facts.bFirstDistrictIntegrated =
        bFirstPublicDistrictAvailable && FirstDistrictLink != 0;
    Facts.bSecondDistrictIntegrated =
        bSecondPublicDistrictAvailable && SecondDistrictLink != 0;
    Facts.bRecordedProtocolApplied =
        Facts.bFutureWellIntact && Well->owner == LocalPlayerId &&
        Well->wellChoice == Plan.LumeProtocol;
    Facts.bConflictingProtocolApplied =
        Facts.bFutureWellIntact &&
        Well->wellChoice != FutureWellChoice::Dormant &&
        Well->wellChoice != Plan.LumeProtocol;
    const bool bCurrentEvidenceAttestation =
        bMeridianEvidenceAvailable && bKharuunEvidenceAvailable &&
        Facts.bOruunIntact && Facts.bLedgerWitnessIntact &&
        IsWithinTiles(
            Oruun->position,
            Plan.KharuunEvidenceSite,
            NoNeutralLedgerSiteRadiusTiles) &&
        IsWithinTiles(
            Witness->position,
            Plan.MeridianEvidenceSite,
            NoNeutralLedgerSiteRadiusTiles);
    Facts.bBothEvidenceChannelsAttested =
        Facts.bRecordedProtocolApplied || bCurrentEvidenceAttestation;
    Facts.bCoalitionRallied =
        Facts.bRecordedProtocolApplied && Facts.bOruunIntact &&
        Facts.bLedgerWitnessIntact &&
        IsWithinTiles(
            Oruun->position,
            Plan.RallySite,
            NoNeutralLedgerSiteRadiusTiles) &&
        IsWithinTiles(
            Witness->position,
            Plan.RallySite,
            NoNeutralLedgerSiteRadiusTiles);
    Facts.bReshapeWindowExpired =
        Facts.bRecordedProtocolApplied &&
        Plan.LumeProtocol == FutureWellChoice::Reshape &&
        Well->reshapeUntilTick == 0 && !Facts.bCoalitionRallied;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts);
}

EEchoesFutureThatWonPhase
UEchoesSimulationSubsystem::GetFutureThatWonPhase() const
{
    FEchoesFutureThatWonMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignFutureThatWon &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesFutureThatWonPhase::Inactive;
    }

    const FEchoesFutureThatWonPlan Plan = GetFutureThatWonPlan();
    const echoes::sim::Entity* Oruun =
        Simulation->FindEntity(FutureWonOruunId);
    const echoes::sim::Entity* Verifier =
        Simulation->FindEntity(FutureWonVerifierId);
    const echoes::sim::Entity* FirstDistrict =
        Simulation->FindEntity(FutureWonFirstDistrictInterfaceId);
    const echoes::sim::Entity* SecondDistrict =
        Simulation->FindEntity(FutureWonSecondDistrictInterfaceId);
    const echoes::sim::Entity* MeridianReadback =
        Simulation->FindEntity(FutureWonMeridianReadbackInterfaceId);
    const echoes::sim::Entity* KharuunReadback =
        Simulation->FindEntity(FutureWonKharuunReadbackInterfaceId);
    const echoes::sim::Entity* Demonstrator =
        Simulation->FindEntity(FutureWonDemonstratorInterfaceId);
    const echoes::sim::Entity* Well =
        Simulation->FindEntity(FutureWonWellId);

    Facts.bOruunIntact = Oruun != nullptr && Oruun->hitPoints > 0;
    Facts.bVerifierIntact =
        Verifier != nullptr && Verifier->hitPoints > 0;
    Facts.bFutureWellIntact =
        Well != nullptr && Well->hitPoints > 0 &&
        Well->type == EntityType::FutureWell &&
        Well->position == Plan.FutureWellSite;
    const bool bFirstDistrictAvailable = IsPublicInterface(
        FirstDistrict,
        Faction::MeridianCompact,
        Plan.FirstDistrictInputSite,
        true);
    const bool bSecondDistrictAvailable = IsPublicInterface(
        SecondDistrict,
        Faction::MeridianCompact,
        Plan.SecondDistrictInputSite,
        true);
    const bool bMeridianReadbackAvailable = IsPublicInterface(
        MeridianReadback,
        Faction::MeridianCompact,
        Plan.MeridianReadbackSite,
        true);
    const bool bKharuunReadbackAvailable = IsPublicInterface(
        KharuunReadback,
        Faction::KharuunAssemblies,
        Plan.KharuunReadbackSite,
        false);
    const bool bDemonstratorAvailable = IsPublicInterface(
        Demonstrator,
        Faction::MeridianCompact,
        Plan.RestorationDemonstratorSite,
        true);
    Facts.bPublicInterfacesIntact =
        bFirstDistrictAvailable && bSecondDistrictAvailable &&
        bMeridianReadbackAvailable && bKharuunReadbackAvailable &&
        bDemonstratorAvailable;

    EntityId FirstInputLink = 0;
    EntityId SecondInputLink = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure)
        {
            if (FirstInputLink == 0 &&
                IsWithinTiles(
                    Entity.position,
                    Plan.FirstDistrictInputSite,
                    FutureThatWonSiteRadiusTiles))
            {
                FirstInputLink = Entity.id;
            }
            if (SecondInputLink == 0 && Entity.id != FirstInputLink &&
                IsWithinTiles(
                    Entity.position,
                    Plan.SecondDistrictInputSite,
                    FutureThatWonSiteRadiusTiles))
            {
                SecondInputLink = Entity.id;
            }
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
    }
    Facts.bFirstRecordedInputVerified =
        bFirstDistrictAvailable && FirstInputLink != 0;
    Facts.bSecondRecordedInputVerified =
        bSecondDistrictAvailable && SecondInputLink != 0;
    Facts.bRecordedProtocolBound =
        Facts.bFutureWellIntact && Well->owner == LocalPlayerId &&
        Well->wellChoice == Plan.RecordedProtocol &&
        Well->wellActivationTick != 0;
    Facts.bConflictingProtocolBound =
        Facts.bFutureWellIntact &&
        Well->wellChoice != FutureWellChoice::Dormant &&
        Well->wellChoice != Plan.RecordedProtocol;
    const bool bCurrentIndependentReadback =
        Facts.bOruunIntact && Facts.bVerifierIntact &&
        bMeridianReadbackAvailable && bKharuunReadbackAvailable &&
        IsWithinTiles(
            Oruun->position,
            Plan.KharuunReadbackSite,
            FutureThatWonSiteRadiusTiles) &&
        IsWithinTiles(
            Verifier->position,
            Plan.MeridianReadbackSite,
            FutureThatWonSiteRadiusTiles);
    Facts.bIndependentPublicReadbackEstablished =
        Facts.bRecordedProtocolBound || bCurrentIndependentReadback;
    Facts.bStabilityWindowHeld =
        Facts.bRecordedProtocolBound &&
        Simulation->CurrentTick() >= Well->wellActivationTick &&
        Simulation->CurrentTick() - Well->wellActivationTick >=
            Plan.StabilityWindowTicks;
    Facts.bFirstDistrictReadbackObserved =
        Facts.bStabilityWindowHeld && Facts.bOruunIntact &&
        IsWithinTiles(
            Oruun->position,
            Plan.FirstDistrictInputSite,
            FutureThatWonSiteRadiusTiles);
    Facts.bSecondDistrictReadbackObserved =
        Facts.bStabilityWindowHeld && Facts.bVerifierIntact &&
        IsWithinTiles(
            Verifier->position,
            Plan.SecondDistrictInputSite,
            FutureThatWonSiteRadiusTiles);
    Facts.bReshapeWindowExpired =
        Facts.bRecordedProtocolBound &&
        Plan.RecordedProtocol == FutureWellChoice::Reshape &&
        Well->reshapeUntilTick == 0 && !Facts.bStabilityWindowHeld;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesFutureThatWonMissionModel::DeterminePhase(Facts);
}

EEchoesAssemblyOfTheMissingPhase
UEchoesSimulationSubsystem::GetAssemblyOfTheMissingPhase() const
{
    FEchoesAssemblyOfTheMissingMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
        bScenarioReady && Simulation.IsValid() &&
        IsAssemblyOfTheMissingUnlocked();
    if (!Facts.bOperationActive)
    {
        return EEchoesAssemblyOfTheMissingPhase::Inactive;
    }

    const FEchoesAssemblyOfTheMissingPlan Plan =
        GetAssemblyOfTheMissingPlan();
    const echoes::sim::Entity* Oruun =
        Simulation->FindEntity(AssemblyOruunId);
    const echoes::sim::Entity* Verifier =
        Simulation->FindEntity(AssemblyVerifierId);
    const bool bMeridianPublicRecordAvailable = IsPublicInterface(
        Simulation->FindEntity(
            AssemblyMeridianPublicRecordInterfaceId),
        Faction::MeridianCompact,
        Plan.MeridianPublicRecordSite,
        true);
    const bool bKharuunPublicRecordAvailable = IsPublicInterface(
        Simulation->FindEntity(
            AssemblyKharuunPublicRecordInterfaceId),
        Faction::KharuunAssemblies,
        Plan.KharuunPublicRecordSite,
        false);
    const bool bCrownfallIndexAvailable = IsPublicInterface(
        Simulation->FindEntity(AssemblyCrownfallIndexInterfaceId),
        Faction::MeridianCompact,
        Plan.CrownfallIndexSite,
        true);
    Facts.bOruunIntact = Oruun != nullptr && Oruun->hitPoints > 0;
    Facts.bVerifierIntact =
        Verifier != nullptr && Verifier->hitPoints > 0;
    Facts.bPublicInterfacesIntact =
        bMeridianPublicRecordAvailable &&
        bKharuunPublicRecordAvailable && bCrownfallIndexAvailable;

    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure &&
            bCrownfallIndexAvailable &&
            IsWithinTiles(
                Entity.position,
                Plan.CrownfallIndexSite,
                AssemblyOfTheMissingSiteRadiusTiles))
        {
            Facts.bCrownfallIndexLinked = true;
        }
    }

    const bool bCurrentPublicRecordReadback =
        Facts.bOruunIntact && Facts.bVerifierIntact &&
        bMeridianPublicRecordAvailable &&
        bKharuunPublicRecordAvailable &&
        IsWithinTiles(
            Oruun->position,
            Plan.KharuunPublicRecordSite,
            AssemblyOfTheMissingSiteRadiusTiles) &&
        IsWithinTiles(
            Verifier->position,
            Plan.MeridianPublicRecordSite,
            AssemblyOfTheMissingSiteRadiusTiles);
    Facts.bPublicRecordReadbackEstablished =
        Facts.bCrownfallIndexLinked || bCurrentPublicRecordReadback;
    Facts.bMeridianAssemblyWitnessObserved =
        Facts.bCrownfallIndexLinked && Facts.bVerifierIntact &&
        IsWithinTiles(
            Verifier->position,
            Plan.MeridianAssemblyWitnessSite,
            AssemblyOfTheMissingSiteRadiusTiles);
    Facts.bKharuunAssemblyWitnessObserved =
        Facts.bCrownfallIndexLinked && Facts.bOruunIntact &&
        IsWithinTiles(
            Oruun->position,
            Plan.KharuunAssemblyWitnessSite,
            AssemblyOfTheMissingSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts);
}

EEchoesSeveralVoicesOneCommandPhase
UEchoesSimulationSubsystem::GetSeveralVoicesOneCommandPhase() const
{
    FEchoesSeveralVoicesOneCommandMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        bScenarioReady && Simulation.IsValid() &&
        IsSeveralVoicesOneCommandUnlocked();
    if (!Facts.bOperationActive)
    {
        return EEchoesSeveralVoicesOneCommandPhase::Inactive;
    }

    const FEchoesSeveralVoicesOneCommandPlan Plan =
        GetSeveralVoicesOneCommandPlan();
    const echoes::sim::Entity* PossibleVoice =
        Simulation->FindEntity(SeveralVoicesPossibleVoiceId);
    const echoes::sim::Entity* ManifestVoice =
        Simulation->FindEntity(SeveralVoicesManifestVoiceId);
    const echoes::sim::Entity* Neme =
        Simulation->FindEntity(SeveralVoicesNemeId);
    const echoes::sim::Entity* ResearchLoom =
        Simulation->FindEntity(SeveralVoicesResearchLoomId);
    const auto IsProtectedChoirEntity = [](
        const echoes::sim::Entity* Entity,
        EntityType Type)
    {
        return Entity != nullptr && Entity->owner == LocalPlayerId &&
               Entity->faction == Faction::HollowChoir &&
               Entity->type == Type && Entity->hitPoints > 0 &&
               Entity->completed;
    };

    Facts.bPossibleVoiceIntact =
        IsProtectedChoirEntity(PossibleVoice, EntityType::Soldier) &&
        PossibleVoice->choirIdentityState !=
            echoes::sim::ChoirIdentityState::NotChoir;
    Facts.bManifestVoiceIntact =
        IsProtectedChoirEntity(ManifestVoice, EntityType::HeavyUnit) &&
        ManifestVoice->choirIdentityState !=
            echoes::sim::ChoirIdentityState::NotChoir;
    Facts.bNemeIntact =
        IsProtectedChoirEntity(Neme, EntityType::ScoutUnit) &&
        Neme->choirIdentityState !=
            echoes::sim::ChoirIdentityState::NotChoir;
    Facts.bResearchLoomIntact =
        IsProtectedChoirEntity(ResearchLoom, EntityType::Barracks);

    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.faction == Faction::HollowChoir &&
            Entity.type == EntityType::CommandCore &&
            Entity.hitPoints > 0 && Entity.completed)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.owner == LocalPlayerId &&
            Entity.faction == Faction::HollowChoir &&
            Entity.type == EntityType::UtilityStructure &&
            Entity.hitPoints > 0 && Entity.completed &&
            IsWithinTiles(
                Entity.position,
                Plan.CrisisAnchorSite,
                SeveralVoicesOneCommandSiteRadiusTiles))
        {
            Facts.bPhaseAnchorComplete = true;
            const uint64 UpkeepInterval =
                Simulation->Config()
                    .rules.choirCoherence.upkeepIntervalTicks;
            if (UpkeepInterval > 0 &&
                Entity.choirCoherenceNextChargeTick >= UpkeepInterval)
            {
                const uint64 CompletionTick =
                    Entity.choirCoherenceNextChargeTick - UpkeepInterval;
                Facts.bCrisisWindowHeld =
                    Simulation->CurrentTick() >= CompletionTick &&
                    Simulation->CurrentTick() - CompletionTick >=
                        SeveralVoicesCrisisHoldTicks;
            }
        }
    }

    const echoes::sim::PlayerState* Player =
        Simulation->FindPlayer(LocalPlayerId);
    Facts.bHeldAlternativesResearched =
        Player != nullptr && Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirHeldAlternatives);
    Facts.bSharedResolutionResearched =
        Player != nullptr && Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirSharedResolution);
    Facts.bPossibleVoiceResolved =
        Facts.bPossibleVoiceIntact &&
        PossibleVoice->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Possible;
    Facts.bPossibleVoiceAtSite =
        Facts.bPossibleVoiceResolved &&
        IsWithinTiles(
            PossibleVoice->position,
            Plan.PossibleVoiceSite,
            SeveralVoicesOneCommandSiteRadiusTiles);
    Facts.bManifestVoiceResolved =
        Facts.bManifestVoiceIntact &&
        ManifestVoice->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Manifest;
    Facts.bManifestVoiceAtSite =
        Facts.bManifestVoiceResolved &&
        IsWithinTiles(
            ManifestVoice->position,
            Plan.ManifestVoiceSite,
            SeveralVoicesOneCommandSiteRadiusTiles);
    Facts.bNemeAtCommandSite =
        Facts.bNemeIntact &&
        IsWithinTiles(
            Neme->position,
            Plan.NemeCommandSite,
            SeveralVoicesOneCommandSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    Facts.bCrisisContractFailed =
        bSeveralVoicesCrisisContractFailed;
    return FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts);
}

EEchoesBrokenSunPhase UEchoesSimulationSubsystem::GetBrokenSunPhase() const
{
    FEchoesBrokenSunMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun &&
        bScenarioReady && Simulation.IsValid() && IsBrokenSunUnlocked();
    if (!Facts.bOperationActive)
    {
        return EEchoesBrokenSunPhase::Inactive;
    }

    const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
    const echoes::sim::Vec2 ResolutionSite =
        FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
            Plan,
            SelectedBrokenSunResolution);
    const uint64 RequiredResolutionTicks =
        FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
            Plan,
            SelectedBrokenSunResolution);
    const echoes::sim::Entity* Voice =
        Simulation->FindEntity(BrokenSunAccordVoiceId);
    const echoes::sim::Entity* Heavy =
        Simulation->FindEntity(BrokenSunAccordHeavyId);
    const echoes::sim::Entity* Neme =
        Simulation->FindEntity(BrokenSunNemeId);
    const echoes::sim::Entity* Worker =
        Simulation->FindEntity(BrokenSunWorkerId);
    const echoes::sim::Entity* Mara =
        Simulation->FindEntity(BrokenSunMaraId);
    const echoes::sim::Entity* Oruun =
        Simulation->FindEntity(BrokenSunOruunId);
    const echoes::sim::Entity* Talar =
        Simulation->FindEntity(BrokenSunTalarId);
    const auto IsEntity = [](
        const echoes::sim::Entity* Entity,
        uint8 Owner,
        Faction EntityFaction,
        EntityType Type)
    {
        return Entity != nullptr && Entity->owner == Owner &&
               Entity->faction == EntityFaction && Entity->type == Type &&
               Entity->hitPoints > 0 && Entity->completed;
    };
    const bool bVoiceIntact = IsEntity(
        Voice,
        LocalPlayerId,
        Faction::HollowChoir,
        EntityType::Soldier);
    const bool bHeavyIntact = IsEntity(
        Heavy,
        LocalPlayerId,
        Faction::HollowChoir,
        EntityType::HeavyUnit);
    Facts.bNemeIntact = IsEntity(
        Neme,
        LocalPlayerId,
        Faction::HollowChoir,
        EntityType::ScoutUnit);
    const bool bWorkerIntact = IsEntity(
        Worker,
        LocalPlayerId,
        Faction::HollowChoir,
        EntityType::Worker);
    Facts.bCommandForceIntact =
        bVoiceIntact && bHeavyIntact && bWorkerIntact;
    Facts.bMaraIntact = IsEntity(
        Mara,
        2,
        Faction::MeridianCompact,
        EntityType::ScoutUnit) &&
        IsWithinTiles(
            Mara->position,
            Plan.MaraAccordSite,
            BrokenSunSiteRadiusTiles);
    Facts.bOruunIntact = IsEntity(
        Oruun,
        3,
        Faction::KharuunAssemblies,
        EntityType::ScoutUnit) &&
        IsWithinTiles(
            Oruun->position,
            Plan.OruunAccordSite,
            BrokenSunSiteRadiusTiles);
    Facts.bTalarIntact = IsEntity(
        Talar,
        2,
        Faction::MeridianCompact,
        EntityType::Worker) &&
        IsWithinTiles(
            Talar->position,
            Plan.TalarPublicRecordSite,
            BrokenSunSiteRadiusTiles);

    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner != LocalPlayerId ||
            Entity.faction != Faction::HollowChoir ||
            Entity.hitPoints <= 0 || !Entity.completed)
        {
            continue;
        }
        Facts.bLocalCoreIntact |=
            Entity.type == EntityType::CommandCore;
        if (Entity.type != EntityType::UtilityStructure)
        {
            continue;
        }
        const bool bAtApproach = IsWithinTiles(
            Entity.position,
            Plan.CrownfallApproachSite,
            BrokenSunSiteRadiusTiles);
        const bool bAtConvergence =
            SelectedBrokenSunResolution !=
                EEchoesFinalResolution::None &&
            IsWithinTiles(
                Entity.position,
                ResolutionSite,
                BrokenSunConvergenceRadiusTiles);
        Facts.bApproachAnchorComplete |=
            bAtApproach &&
            (BrokenSunApproachAnchorId == 0 ||
             Entity.id == BrokenSunApproachAnchorId);
        Facts.bResolutionConduitComplete |=
            bAtConvergence &&
            (BrokenSunResolutionConduitId == 0 ||
             Entity.id == BrokenSunResolutionConduitId);
    }

    const echoes::sim::PlayerState* Player =
        Simulation->FindPlayer(LocalPlayerId);
    const bool bAccordResearchComplete =
        Player != nullptr &&
        Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirHeldAlternatives) &&
        Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirSharedResolution);
    Facts.bMeridianAccordEstablished =
        bAccordResearchComplete && bVoiceIntact &&
        Voice->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Possible &&
        IsWithinTiles(
            Voice->position,
            Plan.MaraAccordSite,
            BrokenSunSiteRadiusTiles);
    Facts.bKharuunAccordEstablished =
        bAccordResearchComplete && bHeavyIntact &&
        Heavy->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Manifest &&
        IsWithinTiles(
            Heavy->position,
            Plan.OruunAccordSite,
            BrokenSunSiteRadiusTiles);
    Facts.bChoirAccordEstablished =
        bAccordResearchComplete && Facts.bNemeIntact &&
        Neme->choirIdentityState !=
            echoes::sim::ChoirIdentityState::NotChoir &&
        IsWithinTiles(
            Neme->position,
            Plan.NemeAccordSite,
            BrokenSunSiteRadiusTiles);
    Facts.SelectedResolution = SelectedBrokenSunResolution;
    Facts.bSelectedResolutionEligible =
        FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            Plan,
            SelectedBrokenSunResolution);
    Facts.bResolutionWindowHeld =
        bBrokenSunResolutionHoldStarted &&
        BrokenSunResolutionStartTick > 0 &&
        Simulation->CurrentTick() >= BrokenSunResolutionStartTick &&
        Simulation->CurrentTick() - BrokenSunResolutionStartTick >=
            RequiredResolutionTicks;
    Facts.bResolutionContractFailed =
        bBrokenSunResolutionContractFailed;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesBrokenSunMissionModel::DeterminePhase(Facts);
}

FEchoesObjectiveSnapshot
UEchoesSimulationSubsystem::GetLocalObjectiveSnapshot() const
{
    FEchoesObjectiveSnapshot Snapshot;
    Snapshot.bScenarioReady = bScenarioReady && Simulation.IsValid();
    if (!Snapshot.bScenarioReady)
    {
        return Snapshot;
    }

    Snapshot.Outcome = Simulation->Outcome();
    Snapshot.OperationMode = SelectedOperation;
    Snapshot.ProloguePhase = GetProloguePhase();
    Snapshot.SevenAccountsPhase = GetSevenAccountsPhase();
    Snapshot.SevenAccountsBranch = GetRecordedPrologueChoice();
    Snapshot.CityReservePhase = GetCityReservePhase();
    Snapshot.CityReserveBranch = GetRecordedPrologueChoice();
    Snapshot.UnburiedRoadPhase = GetUnburiedRoadPhase();
    Snapshot.UnburiedRoadBranch = GetRecordedPrologueChoice();
    Snapshot.TermsOfContinuancePhase = GetTermsOfContinuancePhase();
    Snapshot.TermsOfContinuanceBranch = GetRecordedPrologueChoice();
    Snapshot.NamesWithoutBirthsPhase = GetNamesWithoutBirthsPhase();
    Snapshot.NamesWithoutBirthsBranch = GetRecordedPrologueChoice();
    Snapshot.ShapeOfSilencePhase = GetShapeOfSilencePhase();
    Snapshot.ShapeOfSilenceBranch = GetRecordedPrologueChoice();
    Snapshot.ShapeBesideUsPhase = GetShapeBesideUsPhase();
    Snapshot.ShapeBesideUsBranch = GetRecordedPrologueChoice();
    Snapshot.ReserveAuthorityPhase = GetReserveAuthorityPhase();
    Snapshot.ReserveAuthorityBranch = GetRecordedPrologueChoice();
    Snapshot.ChoirAtLumeReachPhase = GetChoirAtLumeReachPhase();
    Snapshot.ChoirAtLumeReachPriorBranch = GetRecordedPrologueChoice();
    Snapshot.NoNeutralLedgerPhase = GetNoNeutralLedgerPhase();
    Snapshot.FutureThatWonPhase = GetFutureThatWonPhase();
    Snapshot.AssemblyOfTheMissingPhase =
        GetAssemblyOfTheMissingPhase();
    Snapshot.SeveralVoicesOneCommandPhase =
        GetSeveralVoicesOneCommandPhase();
    Snapshot.BrokenSunPhase = GetBrokenSunPhase();
    Snapshot.BrokenSunPendingFinalResolution =
        PendingBrokenSunResolution;
    Snapshot.BrokenSunFinalResolution = SelectedBrokenSunResolution;
    Snapshot.ArchiveCarrierId = ArchiveCarrierId;
    Snapshot.MemoryBearerId = MemoryBearerId;
    Snapshot.MigrationWaystoneId = MigrationWaystoneId;
    Snapshot.LifeSupportDistrictId = LifeSupportDistrictId;
    Snapshot.TransitDistrictId = TransitDistrictId;
    Snapshot.ArchiveDistrictId = ArchiveDistrictId;
    Snapshot.MeridianContinuanceRelayId = MeridianContinuanceRelayId;
    Snapshot.KharuunContinuanceSpineId = KharuunContinuanceSpineId;
    Snapshot.MeridianContinuanceWitnessId = MeridianContinuanceWitnessId;
    Snapshot.KharuunContinuanceWitnessId = KharuunContinuanceWitnessId;
    Snapshot.TalarId = TalarId;
    Snapshot.CensusArchiveId = CensusArchiveId;
    Snapshot.FirstCivilianId = FirstCivilianId;
    Snapshot.SecondCivilianId = SecondCivilianId;
    Snapshot.OruunId = OruunId;
    Snapshot.FirstMemoryWitnessId = FirstMemoryWitnessId;
    Snapshot.SecondMemoryWitnessId = SecondMemoryWitnessId;
    Snapshot.ShapeBesideUsTalarId = ShapeBesideUsTalarId;
    Snapshot.FirstStateWitnessId = FirstStateWitnessId;
    Snapshot.SecondStateWitnessId = SecondStateWitnessId;
    Snapshot.ReserveAuthorityMaraId = ReserveAuthorityMaraId;
    Snapshot.ChoirAtLumeReachOruunId = ChoirAtLumeReachOruunId;
    Snapshot.ChoirAtLumeReachWaystoneId = ChoirAtLumeReachWaystoneId;
    Snapshot.ChoirAtLumeReachWellId = ChoirAtLumeReachWellId;
    Snapshot.NoNeutralOruunId = NoNeutralOruunId;
    Snapshot.NoNeutralWaystoneId = NoNeutralWaystoneId;
    Snapshot.NoNeutralLedgerWitnessId = NoNeutralLedgerWitnessId;
    Snapshot.NoNeutralFirstDistrictInterfaceId =
        NoNeutralFirstDistrictInterfaceId;
    Snapshot.NoNeutralSecondDistrictInterfaceId =
        NoNeutralSecondDistrictInterfaceId;
    Snapshot.NoNeutralMeridianEvidenceInterfaceId =
        NoNeutralMeridianEvidenceInterfaceId;
    Snapshot.NoNeutralKharuunEvidenceInterfaceId =
        NoNeutralKharuunEvidenceInterfaceId;
    Snapshot.NoNeutralWellId = NoNeutralWellId;
    Snapshot.FutureWonOruunId = FutureWonOruunId;
    Snapshot.FutureWonVerifierId = FutureWonVerifierId;
    Snapshot.FutureWonFirstDistrictInterfaceId =
        FutureWonFirstDistrictInterfaceId;
    Snapshot.FutureWonSecondDistrictInterfaceId =
        FutureWonSecondDistrictInterfaceId;
    Snapshot.FutureWonMeridianReadbackInterfaceId =
        FutureWonMeridianReadbackInterfaceId;
    Snapshot.FutureWonKharuunReadbackInterfaceId =
        FutureWonKharuunReadbackInterfaceId;
    Snapshot.FutureWonDemonstratorInterfaceId =
        FutureWonDemonstratorInterfaceId;
    Snapshot.FutureWonWellId = FutureWonWellId;
    Snapshot.AssemblyOruunId = AssemblyOruunId;
    Snapshot.AssemblyVerifierId = AssemblyVerifierId;
    Snapshot.AssemblyMeridianPublicRecordInterfaceId =
        AssemblyMeridianPublicRecordInterfaceId;
    Snapshot.AssemblyKharuunPublicRecordInterfaceId =
        AssemblyKharuunPublicRecordInterfaceId;
    Snapshot.AssemblyCrownfallIndexInterfaceId =
        AssemblyCrownfallIndexInterfaceId;
    Snapshot.SeveralVoicesPossibleVoiceId =
        SeveralVoicesPossibleVoiceId;
    Snapshot.SeveralVoicesManifestVoiceId =
        SeveralVoicesManifestVoiceId;
    Snapshot.SeveralVoicesNemeId = SeveralVoicesNemeId;
    Snapshot.SeveralVoicesResearchLoomId =
        SeveralVoicesResearchLoomId;
    const FEchoesSevenAccountsRoute SevenAccountsRoute =
        GetSevenAccountsRoute();
    const FEchoesUnburiedRoadRoute UnburiedRoadRoute =
        GetUnburiedRoadRoute();
    const FEchoesTermsOfContinuancePlan ContinuancePlan =
        GetTermsOfContinuancePlan();
    const FEchoesNamesWithoutBirthsPlan NamesPlan =
        GetNamesWithoutBirthsPlan();
    const FEchoesShapeOfSilencePlan ShapePlan =
        GetShapeOfSilencePlan();
    const FEchoesShapeBesideUsPlan BesidePlan =
        GetShapeBesideUsPlan();
    const FEchoesReserveAuthorityPlan ReservePlan =
        GetReserveAuthorityPlan();
    const FEchoesChoirAtLumeReachPlan ChoirPlan =
        GetChoirAtLumeReachPlan();
    const FEchoesNoNeutralLedgerPlan NoNeutralPlan =
        GetNoNeutralLedgerPlan();
    const FEchoesFutureThatWonPlan FutureWonPlan =
        GetFutureThatWonPlan();
    const FEchoesAssemblyOfTheMissingPlan AssemblyPlan =
        GetAssemblyOfTheMissingPlan();
    const FEchoesSeveralVoicesOneCommandPlan SeveralVoicesPlan =
        GetSeveralVoicesOneCommandPlan();
    const bool bFirstNoNeutralPublicDistrictAvailable =
        IsPublicInterface(
            Simulation->FindEntity(NoNeutralFirstDistrictInterfaceId),
            Faction::MeridianCompact,
            NoNeutralPlan.FirstDistrictSite,
            true);
    const bool bSecondNoNeutralPublicDistrictAvailable =
        IsPublicInterface(
            Simulation->FindEntity(NoNeutralSecondDistrictInterfaceId),
            Faction::MeridianCompact,
            NoNeutralPlan.SecondDistrictSite,
            true);
    const bool bNoNeutralMeridianEvidenceAvailable =
        IsPublicInterface(
            Simulation->FindEntity(NoNeutralMeridianEvidenceInterfaceId),
            Faction::MeridianCompact,
            NoNeutralPlan.MeridianEvidenceSite,
            true);
    const bool bNoNeutralKharuunEvidenceAvailable =
        IsPublicInterface(
            Simulation->FindEntity(NoNeutralKharuunEvidenceInterfaceId),
            Faction::KharuunAssemblies,
            NoNeutralPlan.KharuunEvidenceSite,
            false);
    Snapshot.bNoNeutralPublicInterfacesIntact =
        bFirstNoNeutralPublicDistrictAvailable &&
        bSecondNoNeutralPublicDistrictAvailable &&
        bNoNeutralMeridianEvidenceAvailable &&
        bNoNeutralKharuunEvidenceAvailable;
    const bool bFirstFutureWonDistrictAvailable = IsPublicInterface(
        Simulation->FindEntity(FutureWonFirstDistrictInterfaceId),
        Faction::MeridianCompact,
        FutureWonPlan.FirstDistrictInputSite,
        true);
    const bool bSecondFutureWonDistrictAvailable = IsPublicInterface(
        Simulation->FindEntity(FutureWonSecondDistrictInterfaceId),
        Faction::MeridianCompact,
        FutureWonPlan.SecondDistrictInputSite,
        true);
    const bool bFutureWonMeridianReadbackAvailable = IsPublicInterface(
        Simulation->FindEntity(FutureWonMeridianReadbackInterfaceId),
        Faction::MeridianCompact,
        FutureWonPlan.MeridianReadbackSite,
        true);
    const bool bFutureWonKharuunReadbackAvailable = IsPublicInterface(
        Simulation->FindEntity(FutureWonKharuunReadbackInterfaceId),
        Faction::KharuunAssemblies,
        FutureWonPlan.KharuunReadbackSite,
        false);
    const bool bFutureWonDemonstratorAvailable = IsPublicInterface(
        Simulation->FindEntity(FutureWonDemonstratorInterfaceId),
        Faction::MeridianCompact,
        FutureWonPlan.RestorationDemonstratorSite,
        true);
    Snapshot.bFutureWonPublicInterfacesIntact =
        bFirstFutureWonDistrictAvailable &&
        bSecondFutureWonDistrictAvailable &&
        bFutureWonMeridianReadbackAvailable &&
        bFutureWonKharuunReadbackAvailable &&
        bFutureWonDemonstratorAvailable;
    const bool bAssemblyMeridianPublicRecordAvailable =
        IsPublicInterface(
            Simulation->FindEntity(
                AssemblyMeridianPublicRecordInterfaceId),
            Faction::MeridianCompact,
            AssemblyPlan.MeridianPublicRecordSite,
            true);
    const bool bAssemblyKharuunPublicRecordAvailable =
        IsPublicInterface(
            Simulation->FindEntity(
                AssemblyKharuunPublicRecordInterfaceId),
            Faction::KharuunAssemblies,
            AssemblyPlan.KharuunPublicRecordSite,
            false);
    const bool bAssemblyCrownfallIndexAvailable =
        IsPublicInterface(
            Simulation->FindEntity(
                AssemblyCrownfallIndexInterfaceId),
            Faction::MeridianCompact,
            AssemblyPlan.CrownfallIndexSite,
            true);
    Snapshot.bAssemblyPublicInterfacesIntact =
        bAssemblyMeridianPublicRecordAvailable &&
        bAssemblyKharuunPublicRecordAvailable &&
        bAssemblyCrownfallIndexAvailable;
    Snapshot.NoNeutralFoundingDoctrine =
        NoNeutralPlan.FoundingDoctrine;
    Snapshot.NoNeutralLumeProtocol = NoNeutralPlan.LumeProtocol;
    Snapshot.NoNeutralFirstDistrict =
        NoNeutralPlan.FirstContributingDistrict;
    Snapshot.NoNeutralSecondDistrict =
        NoNeutralPlan.SecondContributingDistrict;
    Snapshot.NoNeutralDeferredDistrict = NoNeutralPlan.DeferredDistrict;
    Snapshot.FutureWonFoundingDoctrine =
        FutureWonPlan.FoundingDoctrine;
    Snapshot.FutureWonRecordedProtocol =
        FutureWonPlan.RecordedProtocol;
    Snapshot.FutureWonFirstDistrict =
        FutureWonPlan.FirstContributingDistrict;
    Snapshot.FutureWonSecondDistrict =
        FutureWonPlan.SecondContributingDistrict;
    Snapshot.FutureWonDeferredDistrict = FutureWonPlan.DeferredDistrict;
    Snapshot.AssemblyFoundingDoctrine =
        AssemblyPlan.FoundingDoctrine;
    Snapshot.AssemblyRecordedProtocol =
        AssemblyPlan.RecordedProtocol;
    Snapshot.AssemblyFirstDistrict =
        AssemblyPlan.FirstContributingDistrict;
    Snapshot.AssemblySecondDistrict =
        AssemblyPlan.SecondContributingDistrict;
    Snapshot.AssemblyDeferredDistrict = AssemblyPlan.DeferredDistrict;
    Snapshot.SeveralVoicesRecordedProtocol =
        SeveralVoicesPlan.RecordedProtocol;
    Snapshot.ChoirAtLumeReachDeferredDistrict = ChoirPlan.DeferredDistrict;
    Snapshot.ReserveAuthorityRecommendedDistrict =
        ReservePlan.RecommendedFirstDistrict;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.id == ArchiveCarrierId)
        {
            Snapshot.bArchiveCarrierIntact = Entity.hitPoints > 0;
            Snapshot.ArchiveCarrierHitPoints = Entity.hitPoints;
        }
        if (Entity.id == MemoryBearerId)
        {
            Snapshot.bMemoryBearerIntact = Entity.hitPoints > 0;
            Snapshot.bMemoryBearerAtAccountSite =
                Snapshot.bMemoryBearerIntact &&
                IsWithinTiles(
                    Entity.position,
                    SevenAccountsRoute.MemoryAccountSite,
                    SevenAccountsSiteRadiusTiles);
            Snapshot.bMemoryBearerAtShard =
                Snapshot.bMemoryBearerIntact &&
                IsWithinTiles(
                    Entity.position,
                    UnburiedRoadRoute.MemoryShardSite,
                    UnburiedRoadSiteRadiusTiles);
        }
        if (Entity.id == MigrationWaystoneId)
        {
            Snapshot.bWaystoneIntact = Entity.hitPoints > 0;
            Snapshot.bWaystoneRootedAtAnchor =
                Snapshot.bWaystoneIntact &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    SevenAccountsRoute.WaystoneAnchor,
                    SevenAccountsSiteRadiusTiles);
            Snapshot.bWaystoneRootedAtRoadhead =
                Snapshot.bWaystoneIntact &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    UnburiedRoadRoute.Roadhead,
                    UnburiedRoadSiteRadiusTiles);
            Snapshot.bShapeWaystoneRooted =
                Snapshot.bWaystoneIntact &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    ShapePlan.WaystoneAnchor,
                    ShapeOfSilenceSiteRadiusTiles);
        }
        if (Entity.id == ChoirAtLumeReachWaystoneId)
        {
            Snapshot.bChoirDeferredLiabilityResolved =
                Entity.hitPoints > 0 &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    ChoirPlan.LiabilitySite,
                    ChoirAtLumeReachSiteRadiusTiles);
        }
        if (Entity.id == NoNeutralWaystoneId)
        {
            Snapshot.bNoNeutralRouteSecured =
                Entity.hitPoints > 0 &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    NoNeutralPlan.RouteSite,
                    NoNeutralLedgerSiteRadiusTiles);
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                UnburiedRoadRoute.ListeningSpineSite,
                UnburiedRoadSiteRadiusTiles))
        {
            Snapshot.bListeningSpineComplete = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                ShapePlan.ListeningSpineSite,
                ShapeOfSilenceSiteRadiusTiles))
        {
            Snapshot.bShapeListeningSpineRaised = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                BesidePlan.EchoRelaySite,
                ShapeBesideUsSiteRadiusTiles))
        {
            Snapshot.bEchoRelayRaised = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                ChoirPlan.FirstAnchorSite,
                ChoirAtLumeReachSiteRadiusTiles))
        {
            Snapshot.bChoirFirstAnchorRaised = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                ChoirPlan.SecondAnchorSite,
                ChoirAtLumeReachSiteRadiusTiles))
        {
            Snapshot.bChoirSecondAnchorRaised = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure &&
            bFirstNoNeutralPublicDistrictAvailable &&
            IsWithinTiles(
                Entity.position,
                NoNeutralPlan.FirstDistrictSite,
                NoNeutralLedgerSiteRadiusTiles))
        {
            Snapshot.bNoNeutralFirstDistrictIntegrated = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure &&
            bSecondNoNeutralPublicDistrictAvailable &&
            IsWithinTiles(
                Entity.position,
                NoNeutralPlan.SecondDistrictSite,
                NoNeutralLedgerSiteRadiusTiles))
        {
            Snapshot.bNoNeutralSecondDistrictIntegrated = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure &&
            bFirstFutureWonDistrictAvailable &&
            IsWithinTiles(
                Entity.position,
                FutureWonPlan.FirstDistrictInputSite,
                FutureThatWonSiteRadiusTiles))
        {
            Snapshot.bFutureWonFirstInputVerified = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure &&
            bSecondFutureWonDistrictAvailable &&
            IsWithinTiles(
                Entity.position,
                FutureWonPlan.SecondDistrictInputSite,
                FutureThatWonSiteRadiusTiles))
        {
            Snapshot.bFutureWonSecondInputVerified = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed &&
            Entity.faction == Faction::KharuunAssemblies &&
            Entity.type == EntityType::UtilityStructure &&
            bAssemblyCrownfallIndexAvailable &&
            IsWithinTiles(
                Entity.position,
                AssemblyPlan.CrownfallIndexSite,
                AssemblyOfTheMissingSiteRadiusTiles))
        {
            Snapshot.bAssemblyCrownfallIndexLinked = true;
        }
        if (Entity.id == LifeSupportDistrictId)
        {
            Snapshot.bLifeSupportPowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == TransitDistrictId)
        {
            Snapshot.bTransitPowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == ArchiveDistrictId)
        {
            Snapshot.bArchivePowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == MeridianContinuanceRelayId)
        {
            Snapshot.bMeridianRelaySynchronized =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == KharuunContinuanceSpineId)
        {
            Snapshot.bKharuunSpineSynchronized =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == MeridianContinuanceWitnessId)
        {
            Snapshot.bMeridianWitnessExtracted =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ContinuancePlan.WitnessExtractionSite,
                    TermsOfContinuanceSiteRadiusTiles);
        }
        if (Entity.id == KharuunContinuanceWitnessId)
        {
            Snapshot.bKharuunWitnessExtracted =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ContinuancePlan.WitnessExtractionSite,
                    TermsOfContinuanceSiteRadiusTiles);
        }
        if (Entity.id == CensusArchiveId)
        {
            Snapshot.bCensusArchivePowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == TalarId)
        {
            Snapshot.bCensusEvidenceLocated =
                Entity.hitPoints > 0 &&
                (Snapshot.bCensusArchivePowered ||
                 IsWithinTiles(
                     Entity.position,
                     NamesPlan.CensusSite,
                     NamesWithoutBirthsSiteRadiusTiles));
        }
        if (Entity.id == FirstCivilianId)
        {
            Snapshot.bFirstCivilianSheltered =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    NamesPlan.CivilianShelterSite,
                    NamesWithoutBirthsSiteRadiusTiles);
        }
        if (Entity.id == SecondCivilianId)
        {
            Snapshot.bSecondCivilianSheltered =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    NamesPlan.CivilianShelterSite,
                    NamesWithoutBirthsSiteRadiusTiles);
        }
        if (Entity.id == FirstMemoryWitnessId)
        {
            Snapshot.bFirstMemoryWitnessPositioned =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ShapePlan.FirstWitnessSite,
                    ShapeOfSilenceSiteRadiusTiles);
        }
        if (Entity.id == SecondMemoryWitnessId)
        {
            Snapshot.bSecondMemoryWitnessPositioned =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ShapePlan.SecondWitnessSite,
                ShapeOfSilenceSiteRadiusTiles);
        }
        if (Entity.id == ShapeBesideUsTalarId)
        {
            Snapshot.bFirstEchoObserved =
                Entity.hitPoints > 0 &&
                (Snapshot.bEchoRelayRaised ||
                 IsWithinTiles(
                     Entity.position,
                     BesidePlan.FirstEchoSite,
                     ShapeBesideUsSiteRadiusTiles));
        }
        if (Entity.id == FirstStateWitnessId)
        {
            Snapshot.bFirstStateTraversed =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    BesidePlan.FirstStateSite,
                    ShapeBesideUsSiteRadiusTiles);
        }
        if (Entity.id == SecondStateWitnessId)
        {
            Snapshot.bSecondStateTraversed =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    BesidePlan.SecondStateSite,
                    ShapeBesideUsSiteRadiusTiles);
        }
        if (Entity.id == ChoirAtLumeReachOruunId)
        {
            Snapshot.bChoirContactEstablished =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ChoirPlan.ContactSite,
                    ChoirAtLumeReachSiteRadiusTiles);
        }
        if (Entity.id == ChoirAtLumeReachWellId)
        {
            Snapshot.ChoirAtLumeReachWellChoice = Entity.wellChoice;
            Snapshot.bChoirReshapeWindowExpired =
                Entity.wellChoice == FutureWellChoice::Reshape &&
                Entity.reshapeUntilTick == 0;
        }
        if (Entity.id == NoNeutralWellId)
        {
            Snapshot.bNoNeutralProtocolApplied =
                Entity.owner == LocalPlayerId &&
                Entity.wellChoice == NoNeutralPlan.LumeProtocol;
            Snapshot.bNoNeutralReshapeWindowExpired =
                Snapshot.bNoNeutralProtocolApplied &&
                NoNeutralPlan.LumeProtocol == FutureWellChoice::Reshape &&
                Entity.reshapeUntilTick == 0;
        }
        if (Entity.id == FutureWonWellId)
        {
            Snapshot.bFutureWonProtocolBound =
                Entity.owner == LocalPlayerId &&
                Entity.wellChoice == FutureWonPlan.RecordedProtocol &&
                Entity.wellActivationTick != 0;
            Snapshot.FutureWonActivationTick = Entity.wellActivationTick;
            if (Entity.wellActivationTick != 0 &&
                MAX_uint64 - Entity.wellActivationTick >=
                    FutureWonPlan.StabilityWindowTicks)
            {
                Snapshot.FutureWonStabilityEndTick =
                    Entity.wellActivationTick +
                    FutureWonPlan.StabilityWindowTicks;
            }
            Snapshot.bFutureWonStabilityWindowHeld =
                Snapshot.bFutureWonProtocolBound &&
                Simulation->CurrentTick() >= Entity.wellActivationTick &&
                Simulation->CurrentTick() - Entity.wellActivationTick >=
                    FutureWonPlan.StabilityWindowTicks;
            Snapshot.bFutureWonReshapeWindowExpired =
                Snapshot.bFutureWonProtocolBound &&
                FutureWonPlan.RecordedProtocol ==
                    FutureWellChoice::Reshape &&
                Entity.reshapeUntilTick == 0 &&
                !Snapshot.bFutureWonStabilityWindowHeld;
        }
        if (Entity.owner == LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore)
        {
            Snapshot.bLocalCoreIntact = true;
            Snapshot.LocalCoreHitPoints = Entity.hitPoints;
            Snapshot.LocalCoreMaxHitPoints = Entity.maxHitPoints;
            continue;
        }
        if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
            Entity.type == echoes::sim::EntityType::FutureWell &&
            (Entity.wellChoice == echoes::sim::FutureWellChoice::Dormant ||
             Entity.owner == LocalPlayerId))
        {
            Snapshot.PrologueWellChoice = Entity.wellChoice;
        }

        const bool bVisible =
            Simulation->IsEntityVisibleTo(LocalPlayerId, Entity.id);
        if (!bVisible)
        {
            continue;
        }
        if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            Snapshot.PrologueWellChoice = Entity.wellChoice;
            Snapshot.bFutureWellVisible = true;
            Snapshot.VisibleFutureWellChoice = Entity.wellChoice;
        }
        else if (Entity.owner != echoes::sim::kNeutralPlayer &&
                 Entity.owner != LocalPlayerId &&
                 Entity.type == echoes::sim::EntityType::CommandCore)
        {
            Snapshot.bHostileCoreVisible = true;
        }
    }
    Snapshot.bContinuanceWindowHeld =
        Simulation->CurrentTick() >=
        ContinuancePlan.ContinuanceWindowEndTick;
    const echoes::sim::Entity* Talar = Simulation->FindEntity(TalarId);
    Snapshot.bTalarAtEvidenceExtraction =
        Talar != nullptr && Talar->hitPoints > 0 &&
        Snapshot.bCensusArchivePowered &&
        Snapshot.bFirstCivilianSheltered &&
        Snapshot.bSecondCivilianSheltered &&
        IsWithinTiles(
            Talar->position,
            NamesPlan.EvidenceExtractionSite,
            NamesWithoutBirthsSiteRadiusTiles);
    const echoes::sim::Entity* Oruun = Simulation->FindEntity(OruunId);
    Snapshot.bOruunAtConfluence =
        Oruun != nullptr && Oruun->hitPoints > 0 &&
        Snapshot.bFirstMemoryWitnessPositioned &&
        Snapshot.bSecondMemoryWitnessPositioned &&
        IsWithinTiles(
            Oruun->position,
            ShapePlan.ConfluenceSite,
            ShapeOfSilenceSiteRadiusTiles);
    const echoes::sim::Entity* ShapeBesideUsTalar =
        Simulation->FindEntity(ShapeBesideUsTalarId);
    Snapshot.bFirstEchoObserved =
        Snapshot.bEchoRelayRaised || Snapshot.bFirstEchoObserved;
    Snapshot.bShapeBesideUsTalarAtConvergence =
        ShapeBesideUsTalar != nullptr &&
        ShapeBesideUsTalar->hitPoints > 0 &&
        Snapshot.bFirstStateTraversed &&
        Snapshot.bSecondStateTraversed &&
        IsWithinTiles(
            ShapeBesideUsTalar->position,
            BesidePlan.ConvergenceSite,
            ShapeBesideUsSiteRadiusTiles);
    FEchoesReserveAuthorityMissionFacts ReserveFacts;
    ReserveFacts.bLifeSupportPowered = Snapshot.bLifeSupportPowered;
    ReserveFacts.bTransitPowered = Snapshot.bTransitPowered;
    ReserveFacts.bArchivePowered = Snapshot.bArchivePowered;
    const int32 ReservePoweredCount =
        FEchoesReserveAuthorityMissionModel::PoweredDistrictCount(
            ReserveFacts);
    const echoes::sim::Entity* ReserveMara =
        Simulation->FindEntity(ReserveAuthorityMaraId);
    Snapshot.bReserveAuthoritySecured =
        ReservePoweredCount > 0 ||
        (ReserveMara != nullptr && ReserveMara->hitPoints > 0 &&
         IsWithinTiles(
             ReserveMara->position,
             ReservePlan.AuthoritySite,
             ReserveAuthoritySiteRadiusTiles));
    Snapshot.ReserveAuthorityDeferredDistrict =
        FEchoesReserveAuthorityMissionModel::DeferredDistrict(ReserveFacts);
    Snapshot.bReserveAuthorityMaraAtDeferredDistrict =
        ReservePoweredCount == 2 && ReserveMara != nullptr &&
        ReserveMara->hitPoints > 0 &&
        IsWithinTiles(
            ReserveMara->position,
            FEchoesCityReserveMissionModel::SiteForDistrict(
                Snapshot.ReserveAuthorityDeferredDistrict),
            ReserveAuthoritySiteRadiusTiles);
    const echoes::sim::Entity* ChoirOruun =
        Simulation->FindEntity(ChoirAtLumeReachOruunId);
    Snapshot.bChoirBranchResolutionCompleted =
        ChoirOruun != nullptr && ChoirOruun->hitPoints > 0 &&
        Snapshot.ChoirAtLumeReachWellChoice != FutureWellChoice::Dormant &&
        IsWithinTiles(
            ChoirOruun->position,
            FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
                Snapshot.ChoirAtLumeReachWellChoice),
            ChoirAtLumeReachSiteRadiusTiles);
    Snapshot.bChoirReshapeWindowExpired =
        Snapshot.bChoirReshapeWindowExpired &&
        !Snapshot.bChoirBranchResolutionCompleted;
    Snapshot.bChoirContactEstablished =
        Snapshot.bChoirContactEstablished ||
        Snapshot.bChoirDeferredLiabilityResolved ||
        Snapshot.bChoirFirstAnchorRaised ||
        Snapshot.bChoirSecondAnchorRaised ||
        Snapshot.ChoirAtLumeReachWellChoice != FutureWellChoice::Dormant;
    const echoes::sim::Entity* NoNeutralOruun =
        Simulation->FindEntity(NoNeutralOruunId);
    const echoes::sim::Entity* NoNeutralWitness =
        Simulation->FindEntity(NoNeutralLedgerWitnessId);
    const bool bCurrentNoNeutralEvidence =
        bNoNeutralMeridianEvidenceAvailable &&
        bNoNeutralKharuunEvidenceAvailable &&
        NoNeutralOruun != nullptr && NoNeutralOruun->hitPoints > 0 &&
        NoNeutralWitness != nullptr && NoNeutralWitness->hitPoints > 0 &&
        IsWithinTiles(
            NoNeutralOruun->position,
            NoNeutralPlan.KharuunEvidenceSite,
            NoNeutralLedgerSiteRadiusTiles) &&
        IsWithinTiles(
            NoNeutralWitness->position,
            NoNeutralPlan.MeridianEvidenceSite,
            NoNeutralLedgerSiteRadiusTiles);
    Snapshot.bNoNeutralEvidenceAttested =
        Snapshot.bNoNeutralProtocolApplied || bCurrentNoNeutralEvidence;
    Snapshot.bNoNeutralCoalitionRallied =
        Snapshot.bNoNeutralProtocolApplied &&
        NoNeutralOruun != nullptr && NoNeutralOruun->hitPoints > 0 &&
        NoNeutralWitness != nullptr && NoNeutralWitness->hitPoints > 0 &&
        IsWithinTiles(
            NoNeutralOruun->position,
            NoNeutralPlan.RallySite,
            NoNeutralLedgerSiteRadiusTiles) &&
        IsWithinTiles(
            NoNeutralWitness->position,
            NoNeutralPlan.RallySite,
            NoNeutralLedgerSiteRadiusTiles);
    Snapshot.bNoNeutralReshapeWindowExpired =
        Snapshot.bNoNeutralReshapeWindowExpired &&
        !Snapshot.bNoNeutralCoalitionRallied;
    const echoes::sim::Entity* FutureWonOruun =
        Simulation->FindEntity(FutureWonOruunId);
    const echoes::sim::Entity* FutureWonVerifier =
        Simulation->FindEntity(FutureWonVerifierId);
    const bool bCurrentFutureWonReadback =
        bFutureWonMeridianReadbackAvailable &&
        bFutureWonKharuunReadbackAvailable &&
        FutureWonOruun != nullptr && FutureWonOruun->hitPoints > 0 &&
        FutureWonVerifier != nullptr && FutureWonVerifier->hitPoints > 0 &&
        IsWithinTiles(
            FutureWonOruun->position,
            FutureWonPlan.KharuunReadbackSite,
            FutureThatWonSiteRadiusTiles) &&
        IsWithinTiles(
            FutureWonVerifier->position,
            FutureWonPlan.MeridianReadbackSite,
            FutureThatWonSiteRadiusTiles);
    Snapshot.bFutureWonIndependentReadbackEstablished =
        Snapshot.bFutureWonProtocolBound || bCurrentFutureWonReadback;
    Snapshot.bFutureWonFirstDistrictReadbackObserved =
        Snapshot.bFutureWonStabilityWindowHeld &&
        FutureWonOruun != nullptr && FutureWonOruun->hitPoints > 0 &&
        IsWithinTiles(
            FutureWonOruun->position,
            FutureWonPlan.FirstDistrictInputSite,
            FutureThatWonSiteRadiusTiles);
    Snapshot.bFutureWonSecondDistrictReadbackObserved =
        Snapshot.bFutureWonStabilityWindowHeld &&
        FutureWonVerifier != nullptr && FutureWonVerifier->hitPoints > 0 &&
        IsWithinTiles(
            FutureWonVerifier->position,
            FutureWonPlan.SecondDistrictInputSite,
            FutureThatWonSiteRadiusTiles);
    const echoes::sim::Entity* AssemblyOruun =
        Simulation->FindEntity(AssemblyOruunId);
    const echoes::sim::Entity* AssemblyVerifier =
        Simulation->FindEntity(AssemblyVerifierId);
    const bool bCurrentAssemblyReadback =
        bAssemblyMeridianPublicRecordAvailable &&
        bAssemblyKharuunPublicRecordAvailable &&
        AssemblyOruun != nullptr && AssemblyOruun->hitPoints > 0 &&
        AssemblyVerifier != nullptr && AssemblyVerifier->hitPoints > 0 &&
        IsWithinTiles(
            AssemblyOruun->position,
            AssemblyPlan.KharuunPublicRecordSite,
            AssemblyOfTheMissingSiteRadiusTiles) &&
        IsWithinTiles(
            AssemblyVerifier->position,
            AssemblyPlan.MeridianPublicRecordSite,
            AssemblyOfTheMissingSiteRadiusTiles);
    Snapshot.bAssemblyPublicRecordReadbackEstablished =
        Snapshot.bAssemblyCrownfallIndexLinked ||
        bCurrentAssemblyReadback;
    Snapshot.bAssemblyMeridianWitnessObserved =
        Snapshot.bAssemblyCrownfallIndexLinked &&
        AssemblyVerifier != nullptr && AssemblyVerifier->hitPoints > 0 &&
        IsWithinTiles(
            AssemblyVerifier->position,
            AssemblyPlan.MeridianAssemblyWitnessSite,
            AssemblyOfTheMissingSiteRadiusTiles);
    Snapshot.bAssemblyKharuunWitnessObserved =
        Snapshot.bAssemblyCrownfallIndexLinked &&
        AssemblyOruun != nullptr && AssemblyOruun->hitPoints > 0 &&
        IsWithinTiles(
            AssemblyOruun->position,
            AssemblyPlan.KharuunAssemblyWitnessSite,
            AssemblyOfTheMissingSiteRadiusTiles);

    const echoes::sim::PlayerState* SeveralVoicesPlayer =
        Simulation->FindPlayer(LocalPlayerId);
    Snapshot.bSeveralVoicesHeldAlternativesResearched =
        SeveralVoicesPlayer != nullptr &&
        SeveralVoicesPlayer->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirHeldAlternatives);
    Snapshot.bSeveralVoicesSharedResolutionResearched =
        SeveralVoicesPlayer != nullptr &&
        SeveralVoicesPlayer->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirSharedResolution);

    const echoes::sim::Entity* SeveralVoicesPossible =
        Simulation->FindEntity(SeveralVoicesPossibleVoiceId);
    if (SeveralVoicesPossible != nullptr &&
        SeveralVoicesPossible->owner == LocalPlayerId &&
        SeveralVoicesPossible->faction == Faction::HollowChoir &&
        SeveralVoicesPossible->type == EntityType::Soldier &&
        SeveralVoicesPossible->hitPoints > 0)
    {
        Snapshot.SeveralVoicesPossibleState =
            SeveralVoicesPossible->choirIdentityState;
        Snapshot.bSeveralVoicesPossibleAtSite =
            SeveralVoicesPossible->choirIdentityState ==
                echoes::sim::ChoirIdentityState::Possible &&
            IsWithinTiles(
                SeveralVoicesPossible->position,
                SeveralVoicesPlan.PossibleVoiceSite,
                SeveralVoicesOneCommandSiteRadiusTiles);
        if (SeveralVoicesPossible->choirIdentityResolveAtTick >
            Simulation->CurrentTick())
        {
            Snapshot.SeveralVoicesPossibleResolveTicksRemaining =
                SeveralVoicesPossible->choirIdentityResolveAtTick -
                Simulation->CurrentTick();
        }
    }

    const echoes::sim::Entity* SeveralVoicesManifest =
        Simulation->FindEntity(SeveralVoicesManifestVoiceId);
    if (SeveralVoicesManifest != nullptr &&
        SeveralVoicesManifest->owner == LocalPlayerId &&
        SeveralVoicesManifest->faction == Faction::HollowChoir &&
        SeveralVoicesManifest->type == EntityType::HeavyUnit &&
        SeveralVoicesManifest->hitPoints > 0)
    {
        Snapshot.SeveralVoicesManifestState =
            SeveralVoicesManifest->choirIdentityState;
        Snapshot.bSeveralVoicesManifestAtSite =
            SeveralVoicesManifest->choirIdentityState ==
                echoes::sim::ChoirIdentityState::Manifest &&
            IsWithinTiles(
                SeveralVoicesManifest->position,
                SeveralVoicesPlan.ManifestVoiceSite,
                SeveralVoicesOneCommandSiteRadiusTiles);
        if (SeveralVoicesManifest->choirIdentityResolveAtTick >
            Simulation->CurrentTick())
        {
            Snapshot.SeveralVoicesManifestResolveTicksRemaining =
                SeveralVoicesManifest->choirIdentityResolveAtTick -
                Simulation->CurrentTick();
        }
    }

    const echoes::sim::Entity* SeveralVoicesNeme =
        Simulation->FindEntity(SeveralVoicesNemeId);
    Snapshot.bSeveralVoicesNemeAtCommandSite =
        SeveralVoicesNeme != nullptr &&
        SeveralVoicesNeme->owner == LocalPlayerId &&
        SeveralVoicesNeme->faction == Faction::HollowChoir &&
        SeveralVoicesNeme->type == EntityType::ScoutUnit &&
        SeveralVoicesNeme->hitPoints > 0 &&
        IsWithinTiles(
            SeveralVoicesNeme->position,
            SeveralVoicesPlan.NemeCommandSite,
            SeveralVoicesOneCommandSiteRadiusTiles);

    const uint64 SeveralVoicesUpkeepInterval =
        Simulation->Config().rules.choirCoherence.upkeepIntervalTicks;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner != LocalPlayerId ||
            Entity.faction != Faction::HollowChoir ||
            Entity.type != EntityType::UtilityStructure ||
            Entity.hitPoints <= 0 || !Entity.completed ||
            !IsWithinTiles(
                Entity.position,
                SeveralVoicesPlan.CrisisAnchorSite,
                SeveralVoicesOneCommandSiteRadiusTiles))
        {
            continue;
        }
        if (Snapshot.SeveralVoicesPhaseAnchorId == 0 ||
            Entity.id < Snapshot.SeveralVoicesPhaseAnchorId)
        {
            Snapshot.SeveralVoicesPhaseAnchorId = Entity.id;
        }
        Snapshot.bSeveralVoicesPhaseAnchorComplete = true;
        if (SeveralVoicesUpkeepInterval == 0 ||
            Entity.choirCoherenceNextChargeTick <
                SeveralVoicesUpkeepInterval)
        {
            continue;
        }
        const uint64 CompletionTick =
            Entity.choirCoherenceNextChargeTick -
            SeveralVoicesUpkeepInterval;
        if (MAX_uint64 - CompletionTick < SeveralVoicesCrisisHoldTicks)
        {
            continue;
        }
        const uint64 CrisisEndTick =
            CompletionTick + SeveralVoicesCrisisHoldTicks;
        if (Simulation->CurrentTick() >= CrisisEndTick)
        {
            Snapshot.bSeveralVoicesCrisisWindowHeld = true;
        }
        else
        {
            const uint64 Remaining =
                CrisisEndTick - Simulation->CurrentTick();
            if (Snapshot.SeveralVoicesCrisisTicksRemaining == 0 ||
                Remaining < Snapshot.SeveralVoicesCrisisTicksRemaining)
            {
                Snapshot.SeveralVoicesCrisisTicksRemaining = Remaining;
            }
        }
    }

    if (SelectedOperation == EEchoesOperationMode::CampaignTheBrokenSun &&
        IsBrokenSunUnlocked())
    {
        const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
        Snapshot.BrokenSunAvailableFinalResolutions =
            Plan.AvailableFinalResolutions;
        Snapshot.BrokenSunAccordVoiceId = BrokenSunAccordVoiceId;
        Snapshot.BrokenSunAccordHeavyId = BrokenSunAccordHeavyId;
        Snapshot.BrokenSunNemeId = BrokenSunNemeId;
        Snapshot.BrokenSunWorkerId = BrokenSunWorkerId;
        Snapshot.BrokenSunMaraId = BrokenSunMaraId;
        Snapshot.BrokenSunOruunId = BrokenSunOruunId;
        Snapshot.BrokenSunTalarId = BrokenSunTalarId;
        Snapshot.BrokenSunApproachAnchorId = BrokenSunApproachAnchorId;
        Snapshot.BrokenSunResolutionConduitId =
            BrokenSunResolutionConduitId;
        Snapshot.bBrokenSunResolutionContractFailed =
            bBrokenSunResolutionContractFailed;

        const auto IsEntity = [](
            const echoes::sim::Entity* Entity,
            uint8 Owner,
            Faction EntityFaction,
            EntityType Type)
        {
            return Entity != nullptr && Entity->owner == Owner &&
                   Entity->faction == EntityFaction &&
                   Entity->type == Type && Entity->hitPoints > 0 &&
                   Entity->completed;
        };
        const echoes::sim::Entity* Voice =
            Simulation->FindEntity(BrokenSunAccordVoiceId);
        const echoes::sim::Entity* Heavy =
            Simulation->FindEntity(BrokenSunAccordHeavyId);
        const echoes::sim::Entity* Neme =
            Simulation->FindEntity(BrokenSunNemeId);
        const echoes::sim::PlayerState* Player =
            Simulation->FindPlayer(LocalPlayerId);
        const bool bResearchComplete =
            Player != nullptr &&
            Player->HasCompletedResearch(
                echoes::sim::ResearchType::ChoirHeldAlternatives) &&
            Player->HasCompletedResearch(
                echoes::sim::ResearchType::ChoirSharedResolution);
        Snapshot.bBrokenSunMeridianAccordEstablished =
            bResearchComplete &&
            IsEntity(
                Voice,
                LocalPlayerId,
                Faction::HollowChoir,
                EntityType::Soldier) &&
            Voice->choirIdentityState ==
                echoes::sim::ChoirIdentityState::Possible &&
            IsWithinTiles(
                Voice->position,
                Plan.MaraAccordSite,
                BrokenSunSiteRadiusTiles);
        Snapshot.bBrokenSunKharuunAccordEstablished =
            bResearchComplete &&
            IsEntity(
                Heavy,
                LocalPlayerId,
                Faction::HollowChoir,
                EntityType::HeavyUnit) &&
            Heavy->choirIdentityState ==
                echoes::sim::ChoirIdentityState::Manifest &&
            IsWithinTiles(
                Heavy->position,
                Plan.OruunAccordSite,
                BrokenSunSiteRadiusTiles);
        Snapshot.bBrokenSunChoirAccordEstablished =
            bResearchComplete &&
            IsEntity(
                Neme,
                LocalPlayerId,
                Faction::HollowChoir,
                EntityType::ScoutUnit) &&
            Neme->choirIdentityState !=
                echoes::sim::ChoirIdentityState::NotChoir &&
            IsWithinTiles(
                Neme->position,
                Plan.NemeAccordSite,
                BrokenSunSiteRadiusTiles);

        const echoes::sim::Vec2 ResolutionSite =
            FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                Plan,
                SelectedBrokenSunResolution);
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner != LocalPlayerId ||
                Entity.faction != Faction::HollowChoir ||
                Entity.type != EntityType::UtilityStructure ||
                Entity.hitPoints <= 0 || !Entity.completed)
            {
                continue;
            }
            if (IsWithinTiles(
                    Entity.position,
                    Plan.CrownfallApproachSite,
                    BrokenSunSiteRadiusTiles) &&
                (BrokenSunApproachAnchorId == 0 ||
                 Entity.id == BrokenSunApproachAnchorId))
            {
                Snapshot.bBrokenSunApproachSecured = true;
                if (Snapshot.BrokenSunApproachAnchorId == 0)
                {
                    Snapshot.BrokenSunApproachAnchorId = Entity.id;
                }
            }
            if (SelectedBrokenSunResolution !=
                    EEchoesFinalResolution::None &&
                IsWithinTiles(
                    Entity.position,
                    ResolutionSite,
                    BrokenSunConvergenceRadiusTiles) &&
                (BrokenSunResolutionConduitId == 0 ||
                 Entity.id == BrokenSunResolutionConduitId))
            {
                Snapshot.bBrokenSunResolutionConduitComplete = true;
                if (Snapshot.BrokenSunResolutionConduitId == 0)
                {
                    Snapshot.BrokenSunResolutionConduitId = Entity.id;
                }
            }
        }

        const uint64 RequiredTicks =
            FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
                Plan,
                SelectedBrokenSunResolution);
        if (bBrokenSunResolutionHoldStarted &&
            BrokenSunResolutionStartTick > 0 && RequiredTicks > 0 &&
            Simulation->CurrentTick() >= BrokenSunResolutionStartTick)
        {
            const uint64 Elapsed =
                Simulation->CurrentTick() - BrokenSunResolutionStartTick;
            Snapshot.bBrokenSunResolutionWindowHeld =
                Elapsed >= RequiredTicks;
            Snapshot.BrokenSunResolutionTicksRemaining =
                Elapsed >= RequiredTicks ? 0 : RequiredTicks - Elapsed;
        }
    }
    return Snapshot;
}

int64 UEchoesSimulationSubsystem::GetSustainedStressCombatHitPoints() const
{
    if (!Simulation.IsValid())
    {
        return 0;
    }
    int64 Total = 0;
    for (const uint32 EntityId : SustainedStressCombatEntityIds)
    {
        if (const echoes::sim::Entity* Entity = Simulation->FindEntity(EntityId))
        {
            Total += FMath::Max(0, Entity->hitPoints);
        }
    }
    return Total;
}

void UEchoesSimulationSubsystem::FailSustainedStressContract(
    const TCHAR* Code,
    const FString& Detail)
{
    if (!bSustainedStressScenario || bSustainedStressFailed)
    {
        return;
    }
    bSustainedStressFailed = true;
    bSimulationPaused = true;
    bScenarioReady = false;
    SustainedStressFailureCode = Code;
    const uint64 Tick = Simulation.IsValid() ? Simulation->CurrentTick() : 0;
    UE_LOG(
        LogEchoes,
        Error,
        TEXT("[ECHOES_STRESS_SUSTAINED_FAILED] code=%s tick=%llu detail=%s"),
        Code,
        static_cast<unsigned long long>(Tick),
        *Detail);
}

bool UEchoesSimulationSubsystem::FindSustainedStressReplacementPosition(
    int32 SlotIndex,
    Vec2& OutPosition) const
{
    if (!Simulation.IsValid() ||
        !SustainedStressCombatSpawnPositions.IsValidIndex(SlotIndex) ||
        !SustainedStressCombatFactions.IsValidIndex(SlotIndex) ||
        !SustainedStressCombatTypes.IsValidIndex(SlotIndex))
    {
        return false;
    }
    const Vec2 Origin = SustainedStressCombatSpawnPositions[SlotIndex];
    for (int32 Radius = 0; Radius <= 12; ++Radius)
    {
        for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
        {
            for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
            {
                if (Radius > 0 &&
                    FMath::Abs(OffsetX) != Radius &&
                    FMath::Abs(OffsetY) != Radius)
                {
                    continue;
                }
                const Vec2 Candidate = Vec2::FromTiles(
                    Origin.x.FloorToInt() + OffsetX,
                    Origin.y.FloorToInt() + OffsetY);
                if (Simulation->IsSpawnPositionAvailable(
                        SustainedStressCombatFactions[SlotIndex],
                        SustainedStressCombatTypes[SlotIndex],
                        Candidate))
                {
                    OutPosition = Candidate;
                    return true;
                }
            }
        }
    }
    return false;
}

bool UEchoesSimulationSubsystem::MaintainSustainedStressContractAfterFixedStep(
    int64 CombatHitPointsBeforeStep)
{
    if (!bSustainedStressScenario)
    {
        return true;
    }
    if (!Simulation.IsValid() || bSustainedStressFailed)
    {
        return false;
    }
    if (Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        FailSustainedStressContract(
            TEXT("TERMINAL_OUTCOME"),
            TEXT("The deterministic match left the ongoing state."));
        return false;
    }
    for (echoes::sim::PlayerId Player = 0;
         Player < echoes::sim::kMaximumPlayers;
         ++Player)
    {
        const echoes::sim::Entity* Core =
            Simulation->FindEntity(SustainedStressCommandCoreIds[Player]);
        if (Core == nullptr || Core->hitPoints <= 0 ||
            Core->owner != Player ||
            Core->type != EntityType::CommandCore)
        {
            FailSustainedStressContract(
                TEXT("COMMAND_CORE_LOST"),
                FString::Printf(TEXT("player=%u"), Player));
            return false;
        }
    }

    const int64 CombatHitPointsAfterStep =
        GetSustainedStressCombatHitPoints();
    if (CombatHitPointsBeforeStep > CombatHitPointsAfterStep)
    {
        SustainedStressIntervalDamage += static_cast<uint64>(
            CombatHitPointsBeforeStep - CombatHitPointsAfterStep);
        SustainedStressLastActivityTick = Simulation->CurrentTick();
    }

    const int32 SlotCount = SustainedStressCombatEntityIds.Num();
    if (SlotCount != 396 ||
        SustainedStressCombatOwners.Num() != SlotCount ||
        SustainedStressCombatFactions.Num() != SlotCount ||
        SustainedStressCombatTypes.Num() != SlotCount ||
        SustainedStressCombatSpawnPositions.Num() != SlotCount)
    {
        FailSustainedStressContract(
            TEXT("SLOT_TABLE_INVALID"),
            FString::Printf(TEXT("slots=%d expected=396"), SlotCount));
        return false;
    }

    constexpr Vec2 TeamDestinations[2][4] = {
        {
            Vec2::FromTiles(46, 46),
            Vec2::FromTiles(18, 46),
            Vec2::FromTiles(46, 18),
            Vec2::FromTiles(18, 18),
        },
        {
            Vec2::FromTiles(18, 18),
            Vec2::FromTiles(46, 18),
            Vec2::FromTiles(18, 46),
            Vec2::FromTiles(46, 46),
        }};
    const int32 DestinationPhase = static_cast<int32>(
        (Simulation->CurrentTick() / PrototypeTicksPerSecond + 1) % 2);
    const auto QueueAttackMove = [this, &TeamDestinations, DestinationPhase](
                                     int32 SlotIndex,
                                     EntityId Actor,
                                     echoes::sim::Tick ExecuteTick,
                                     const TCHAR* FailureCode)
    {
        const uint8 Owner = SustainedStressCombatOwners[SlotIndex];
        const std::optional<uint64> Sequence =
            Simulation->NextCommandSequence(Owner);
        if (!Sequence.has_value() ||
            Simulation->CommandLog().size() >=
                echoes::sim::kMaximumCommandLogEntries)
        {
            FailSustainedStressContract(
                TEXT("COMMAND_CAPACITY_EXHAUSTED"),
                FString::Printf(
                    TEXT("slot=%d commands=%llu capacity=%llu"),
                    SlotIndex,
                    static_cast<unsigned long long>(
                        Simulation->CommandLog().size()),
                    static_cast<unsigned long long>(
                        echoes::sim::kMaximumCommandLogEntries)));
            return false;
        }
        echoes::sim::Command Command;
        Command.executeTick = ExecuteTick;
        Command.player = Owner;
        Command.sequence = *Sequence;
        Command.type = echoes::sim::CommandType::AttackMove;
        Command.actor = Actor;
        Command.position = TeamDestinations[DestinationPhase][Owner];
        std::string Rejection;
        if (!Simulation->QueueCommand(Command, &Rejection))
        {
            FailSustainedStressContract(
                FailureCode,
                FString::Printf(
                    TEXT("slot=%d owner=%u reason=%s"),
                    SlotIndex,
                    Owner,
                    UTF8_TO_TCHAR(Rejection.c_str())));
            return false;
        }
        return true;
    };
    TSet<EntityId> OrderedThisStep;
    TSet<uint8> OwnersOrderedThisStep;
    for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
    {
        const echoes::sim::Entity* Existing = Simulation->FindEntity(
            SustainedStressCombatEntityIds[SlotIndex]);
        if (Existing != nullptr)
        {
            if (Existing->hitPoints <= 0 ||
                Existing->owner != SustainedStressCombatOwners[SlotIndex] ||
                Existing->faction != SustainedStressCombatFactions[SlotIndex] ||
                Existing->type != SustainedStressCombatTypes[SlotIndex])
            {
                FailSustainedStressContract(
                    TEXT("SLOT_IDENTITY_DRIFT"),
                    FString::Printf(TEXT("slot=%d entity=%u"), SlotIndex,
                                    Existing->id));
                return false;
            }
            continue;
        }

        ++SustainedStressIntervalCombatLosses;
        ++SustainedStressCumulativeCombatLosses;
        SustainedStressLastActivityTick = Simulation->CurrentTick();
        if (SustainedStressCumulativeReplacements >=
            SustainedStressMaximumReplacementCommands)
        {
            FailSustainedStressContract(
                TEXT("REPLACEMENT_BUDGET_EXHAUSTED"),
                FString::Printf(
                    TEXT("replacements=%llu budget=%llu"),
                    static_cast<unsigned long long>(
                        SustainedStressCumulativeReplacements),
                    static_cast<unsigned long long>(
                        SustainedStressMaximumReplacementCommands)));
            return false;
        }
        Vec2 ReplacementPosition;
        if (!FindSustainedStressReplacementPosition(
                SlotIndex, ReplacementPosition))
        {
            FailSustainedStressContract(
                TEXT("REPLACEMENT_POSITION_UNAVAILABLE"),
                FString::Printf(TEXT("slot=%d"), SlotIndex));
            return false;
        }
        const uint8 Owner = SustainedStressCombatOwners[SlotIndex];
        const EntityId Replacement = Simulation->SpawnEntity(
            Owner,
            SustainedStressCombatFactions[SlotIndex],
            SustainedStressCombatTypes[SlotIndex],
            ReplacementPosition);
        if (Replacement == 0)
        {
            FailSustainedStressContract(
                TEXT("REPLACEMENT_SPAWN_REJECTED"),
                FString::Printf(TEXT("slot=%d owner=%u"), SlotIndex, Owner));
            return false;
        }
        if (!QueueAttackMove(
                SlotIndex,
                Replacement,
                Simulation->CurrentTick() + 1,
                TEXT("REPLACEMENT_ORDER_REJECTED")))
        {
            return false;
        }
        SustainedStressCombatEntityIds[SlotIndex] = Replacement;
        OrderedThisStep.Add(Replacement);
        OwnersOrderedThisStep.Add(Owner);
        ++SustainedStressIntervalReplacements;
        ++SustainedStressCumulativeReplacements;
        SustainedStressLastActivityTick = Simulation->CurrentTick();
    }
    if ((Simulation->CurrentTick() + 1) % PrototypeTicksPerSecond == 0 &&
        Simulation->CurrentTick() < SustainedStressQualificationTicks)
    {
        for (echoes::sim::PlayerId Player = 0;
             Player < echoes::sim::kMaximumPlayers;
             ++Player)
        {
            // Replacement orders for this step execute one tick later by
            // contract. Do not queue a later sequence for an earlier tick.
            if (OwnersOrderedThisStep.Contains(Player))
            {
                continue;
            }
            int32& Cursor = SustainedStressRenewalCursorByPlayer[Player];
            Cursor = FMath::Clamp(Cursor, 0, SlotCount - 1);
            for (int32 Attempt = 0; Attempt < SlotCount; ++Attempt)
            {
                const int32 SlotIndex = (Cursor + Attempt) % SlotCount;
                if (SustainedStressCombatOwners[SlotIndex] != Player)
                {
                    continue;
                }
                const EntityId Actor = SustainedStressCombatEntityIds[SlotIndex];
                const echoes::sim::Entity* Entity =
                    Simulation->FindEntity(Actor);
                if (Entity == nullptr || OrderedThisStep.Contains(Actor) ||
                    Entity->order.type == echoes::sim::OrderType::AttackMove)
                {
                    continue;
                }
                if (SustainedStressCumulativeOrderRenewals >=
                    SustainedStressMaximumOrderRenewals)
                {
                    FailSustainedStressContract(
                        TEXT("ORDER_RENEWAL_BUDGET_EXHAUSTED"),
                        FString::Printf(
                            TEXT("renewals=%llu budget=%llu"),
                            static_cast<unsigned long long>(
                                SustainedStressCumulativeOrderRenewals),
                            static_cast<unsigned long long>(
                                SustainedStressMaximumOrderRenewals)));
                    return false;
                }
                if (!QueueAttackMove(
                        SlotIndex,
                        Actor,
                        Simulation->CurrentTick(),
                        TEXT("ORDER_RENEWAL_REJECTED")))
                {
                    return false;
                }
                Cursor = (SlotIndex + 1) % SlotCount;
                ++SustainedStressIntervalOrderRenewals;
                ++SustainedStressCumulativeOrderRenewals;
                break;
            }
        }
    }
    return ValidateSustainedStressContract(false, false, false);
}

bool UEchoesSimulationSubsystem::ValidateSustainedStressContract(
    bool bRequireSynchronizedViews,
    bool bRequireRecentActivity,
    bool bEmitHeartbeat)
{
    if (!bSustainedStressScenario)
    {
        return true;
    }
    if (!Simulation.IsValid() || bSustainedStressFailed)
    {
        return false;
    }

    std::array<int32, echoes::sim::kMaximumPlayers> TeamCounts{};
    std::array<int32, echoes::sim::kMaximumPlayers> TeamCoreCounts{};
    std::array<int32, 3> FactionCounts{};
    int32 SoldierCount = 0;
    int32 HeavyCount = 0;
    int32 ScoutCount = 0;
    int32 OwnedEntityCount = 0;
    int32 NeutralDormantWellCount = 0;
    int32 DamagedCombatants = 0;
    int32 ActiveAttackMoveOrders = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner < echoes::sim::kMaximumPlayers)
        {
            ++OwnedEntityCount;
            ++TeamCounts[Entity.owner];
            switch (Entity.faction)
            {
                case Faction::MeridianCompact: ++FactionCounts[0]; break;
                case Faction::KharuunAssemblies: ++FactionCounts[1]; break;
                case Faction::HollowChoir: ++FactionCounts[2]; break;
            }
            switch (Entity.type)
            {
                case EntityType::CommandCore:
                    ++TeamCoreCounts[Entity.owner];
                    break;
                case EntityType::Soldier:
                    ++SoldierCount;
                    DamagedCombatants +=
                        Entity.hitPoints < Entity.maxHitPoints ? 1 : 0;
                    break;
                case EntityType::HeavyUnit:
                    ++HeavyCount;
                    DamagedCombatants +=
                        Entity.hitPoints < Entity.maxHitPoints ? 1 : 0;
                    break;
                case EntityType::ScoutUnit:
                    ++ScoutCount;
                    DamagedCombatants +=
                        Entity.hitPoints < Entity.maxHitPoints ? 1 : 0;
                    break;
                default:
                    FailSustainedStressContract(
                        TEXT("UNEXPECTED_OWNED_TYPE"),
                        FString::Printf(
                            TEXT("entity=%u type=%u"),
                            Entity.id,
                            static_cast<uint8>(Entity.type)));
                    return false;
            }
            ActiveAttackMoveOrders +=
                Entity.type != EntityType::CommandCore &&
                        Entity.order.type == echoes::sim::OrderType::AttackMove
                    ? 1
                    : 0;
        }
        else if (Entity.owner == echoes::sim::kNeutralPlayer &&
                 Entity.type == EntityType::FutureWell &&
                 Entity.wellChoice == FutureWellChoice::Dormant)
        {
            ++NeutralDormantWellCount;
        }
        else
        {
            FailSustainedStressContract(
                TEXT("UNEXPECTED_NEUTRAL_ENTITY"),
                FString::Printf(TEXT("entity=%u"), Entity.id));
            return false;
        }
    }

    int32 ActivePlayers = 0;
    TSet<uint8> ActiveFactions;
    constexpr Faction ExpectedFactions[4] = {
        Faction::MeridianCompact,
        Faction::KharuunAssemblies,
        Faction::HollowChoir,
        Faction::MeridianCompact};
    for (echoes::sim::PlayerId Player = 0;
         Player < echoes::sim::kMaximumPlayers;
         ++Player)
    {
        const echoes::sim::PlayerState* State = Simulation->FindPlayer(Player);
        if (State != nullptr)
        {
            ++ActivePlayers;
            ActiveFactions.Add(static_cast<uint8>(State->faction));
        }
        if (State == nullptr || State->faction != ExpectedFactions[Player] ||
            TeamCounts[Player] != 100 || TeamCoreCounts[Player] != 1)
        {
            FailSustainedStressContract(
                TEXT("TEAM_COMPOSITION_DRIFT"),
                FString::Printf(
                    TEXT("player=%u count=%d cores=%d"),
                    Player,
                    TeamCounts[Player],
                    TeamCoreCounts[Player]));
            return false;
        }
    }
    if (ActivePlayers != 4 || ActiveFactions.Num() != 3 ||
        FactionCounts[0] != 200 || FactionCounts[1] != 100 ||
        FactionCounts[2] != 100 || OwnedEntityCount != 400 ||
        SoldierCount != 132 || HeavyCount != 132 || ScoutCount != 132 ||
        NeutralDormantWellCount != 1 ||
        Simulation->Entities().size() != 401 ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        FailSustainedStressContract(
            TEXT("POPULATION_OR_FACTION_DRIFT"),
            FString::Printf(
                TEXT("players=%d factions=%d meridian=%d kharuun=%d choir=%d owned=%d soldiers=%d heavies=%d scouts=%d wells=%d entities=%llu outcome=%u"),
                ActivePlayers,
                ActiveFactions.Num(),
                FactionCounts[0],
                FactionCounts[1],
                FactionCounts[2],
                OwnedEntityCount,
                SoldierCount,
                HeavyCount,
                ScoutCount,
                NeutralDormantWellCount,
                static_cast<unsigned long long>(Simulation->Entities().size()),
                static_cast<uint8>(Simulation->Outcome())));
        return false;
    }
    const uint64 ExpectedCommandCount =
        SustainedStressInitialCommandCount +
        SustainedStressCumulativeReplacements +
        SustainedStressCumulativeOrderRenewals;
    if (SustainedStressCumulativeCombatLosses !=
            SustainedStressCumulativeReplacements ||
        SustainedStressCumulativeReplacements >
            SustainedStressMaximumReplacementCommands ||
        SustainedStressCumulativeOrderRenewals >
            SustainedStressMaximumOrderRenewals ||
        ExpectedCommandCount > SustainedStressProjectedCommandCeiling ||
        Simulation->CommandLog().size() != ExpectedCommandCount ||
        Simulation->CommandLog().size() >=
            echoes::sim::kMaximumCommandLogEntries)
    {
        FailSustainedStressContract(
            TEXT("COMMAND_BUDGET_INVALID"),
            FString::Printf(
                TEXT("commands=%llu expected=%llu capacity=%llu losses=%llu replacements=%llu renewals=%llu projectedCeiling=%llu"),
                static_cast<unsigned long long>(Simulation->CommandLog().size()),
                static_cast<unsigned long long>(ExpectedCommandCount),
                static_cast<unsigned long long>(
                    echoes::sim::kMaximumCommandLogEntries),
                static_cast<unsigned long long>(
                    SustainedStressCumulativeCombatLosses),
                static_cast<unsigned long long>(
                    SustainedStressCumulativeReplacements),
                static_cast<unsigned long long>(
                    SustainedStressCumulativeOrderRenewals),
                static_cast<unsigned long long>(
                    SustainedStressProjectedCommandCeiling)));
        return false;
    }
    if (bRequireSynchronizedViews)
    {
        if (EntityViews.Num() != 401)
        {
            FailSustainedStressContract(
                TEXT("VIEW_COUNT_DRIFT"),
                FString::Printf(TEXT("views=%d expected=401"), EntityViews.Num()));
            return false;
        }
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            const TWeakObjectPtr<AEchoesEntityView>* View =
                EntityViews.Find(Entity.id);
            if (View == nullptr || !View->IsValid() ||
                !Simulation->IsEntityVisibleTo(LocalPlayerId, Entity.id))
            {
                FailSustainedStressContract(
                    TEXT("VIEW_IDENTITY_DRIFT"),
                    FString::Printf(TEXT("entity=%u"), Entity.id));
                return false;
            }
        }
        for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair :
             EntityViews)
        {
            if (!Pair.Value.IsValid() ||
                Simulation->FindEntity(Pair.Key) == nullptr)
            {
                FailSustainedStressContract(
                    TEXT("STALE_VIEW"),
                    FString::Printf(TEXT("entity=%u"), Pair.Key));
                return false;
            }
        }
    }
    const uint64 CurrentTick = Simulation->CurrentTick();
    const uint64 ActivityAgeTicks =
        CurrentTick >= SustainedStressLastActivityTick
            ? CurrentTick - SustainedStressLastActivityTick
            : MAX_uint64;
    if (bRequireRecentActivity &&
        (ActivityAgeTicks > SustainedStressActivityWindowTicks ||
         ActiveAttackMoveOrders == 0))
    {
        FailSustainedStressContract(
            TEXT("ACTIVITY_STALLED"),
            FString::Printf(
                TEXT("activityAgeTicks=%llu activityWindowTicks=%llu intervalDamage=%llu intervalReplacements=%llu activeAttackMove=%d"),
                static_cast<unsigned long long>(ActivityAgeTicks),
                static_cast<unsigned long long>(
                    SustainedStressActivityWindowTicks),
                static_cast<unsigned long long>(SustainedStressIntervalDamage),
                static_cast<unsigned long long>(
                    SustainedStressIntervalReplacements),
                ActiveAttackMoveOrders));
        return false;
    }
    if (!bEmitHeartbeat)
    {
        return true;
    }

    const uint64 Tick = CurrentTick;
    if (Tick == 0 || Tick % PrototypeTicksPerSecond != 0 ||
        (SustainedStressLastHeartbeatTick != 0 &&
         Tick - SustainedStressLastHeartbeatTick != PrototypeTicksPerSecond))
    {
        FailSustainedStressContract(
            TEXT("HEARTBEAT_CADENCE_INVALID"),
            FString::Printf(
                TEXT("tick=%llu previous=%llu cadence=%u"),
                static_cast<unsigned long long>(Tick),
                static_cast<unsigned long long>(SustainedStressLastHeartbeatTick),
                PrototypeTicksPerSecond));
        return false;
    }
    uint64 WallMs = static_cast<uint64>(FMath::Max(
        0.0,
        (FPlatformTime::Seconds() - SustainedStressReadyWallSeconds) * 1000.0));
    if (WallMs <= SustainedStressLastHeartbeatWallMs)
    {
        WallMs = SustainedStressLastHeartbeatWallMs + 1;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_STRESS_SUSTAINED_HEARTBEAT] fixture=Stress400Sustained tick=%llu wall_ms=%llu checksum=%llu outcome=ongoing activePlayers=4 activeFactions=3 meridian=200 kharuun=100 hollowChoir=100 team0=100 team1=100 team2=100 team3=100 commandCores=4 soldiers=132 heavies=132 scouts=132 combatUnits=396 ownedEntities=400 neutralWells=1 entities=401 views=401 damagedCombatants=%d activeAttackMove=%d activityAgeTicks=%llu activityWindowTicks=%llu intervalDamage=%llu intervalCombatLosses=%llu cumulativeCombatLosses=%llu intervalReplacements=%llu cumulativeReplacements=%llu intervalOrderRenewals=%llu cumulativeOrderRenewals=%llu commandLog=%llu commandCapacity=%llu replacementBudget=%llu renewalBudget=%llu projectedCommandCeiling=%llu commandSafetyReserve=%llu qualificationTicks=%llu"),
        static_cast<unsigned long long>(Tick),
        static_cast<unsigned long long>(WallMs),
        static_cast<unsigned long long>(Simulation->StateChecksum()),
        DamagedCombatants,
        ActiveAttackMoveOrders,
        static_cast<unsigned long long>(ActivityAgeTicks),
        static_cast<unsigned long long>(SustainedStressActivityWindowTicks),
        static_cast<unsigned long long>(SustainedStressIntervalDamage),
        static_cast<unsigned long long>(
            SustainedStressIntervalCombatLosses),
        static_cast<unsigned long long>(
            SustainedStressCumulativeCombatLosses),
        static_cast<unsigned long long>(SustainedStressIntervalReplacements),
        static_cast<unsigned long long>(SustainedStressCumulativeReplacements),
        static_cast<unsigned long long>(
            SustainedStressIntervalOrderRenewals),
        static_cast<unsigned long long>(
            SustainedStressCumulativeOrderRenewals),
        static_cast<unsigned long long>(Simulation->CommandLog().size()),
        static_cast<unsigned long long>(echoes::sim::kMaximumCommandLogEntries),
        static_cast<unsigned long long>(
            SustainedStressMaximumReplacementCommands),
        static_cast<unsigned long long>(SustainedStressMaximumOrderRenewals),
        static_cast<unsigned long long>(
            SustainedStressProjectedCommandCeiling),
        static_cast<unsigned long long>(
            echoes::sim::kMaximumCommandLogEntries -
            SustainedStressProjectedCommandCeiling),
        static_cast<unsigned long long>(SustainedStressQualificationTicks));
    if (!bSustainedStressQualificationLogged &&
        Tick == SustainedStressQualificationTicks)
    {
        bSustainedStressQualificationLogged = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_STRESS_SUSTAINED_QUALIFIED] fixture=Stress400Sustained tick=%llu checksum=%llu outcome=ongoing combatUnits=396 ownedEntities=400 entities=401 views=401 cumulativeCombatLosses=%llu cumulativeReplacements=%llu cumulativeOrderRenewals=%llu commandLog=%llu commandCapacity=%llu"),
            static_cast<unsigned long long>(Tick),
            static_cast<unsigned long long>(Simulation->StateChecksum()),
            static_cast<unsigned long long>(
                SustainedStressCumulativeCombatLosses),
            static_cast<unsigned long long>(
                SustainedStressCumulativeReplacements),
            static_cast<unsigned long long>(
                SustainedStressCumulativeOrderRenewals),
            static_cast<unsigned long long>(Simulation->CommandLog().size()),
            static_cast<unsigned long long>(
                echoes::sim::kMaximumCommandLogEntries));
    }
    SustainedStressLastHeartbeatTick = Tick;
    SustainedStressLastHeartbeatWallMs = WallMs;
    SustainedStressIntervalDamage = 0;
    SustainedStressIntervalCombatLosses = 0;
    SustainedStressIntervalReplacements = 0;
    SustainedStressIntervalOrderRenewals = 0;
    return true;
}

void UEchoesSimulationSubsystem::Tick(float DeltaTime)
{
    if (!bScenarioReady || !Simulation.IsValid() || bSimulationPaused ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        return;
    }

    if (bSustainedStressScenario && !bSustainedStressTimingReady)
    {
        if (!FMath::IsFinite(DeltaTime) || DeltaTime < 0.0F)
        {
            FailSustainedStressContract(
                TEXT("SIM_STARTUP_TIME_INVALID"),
                FString::Printf(
                    TEXT("rawDeltaSeconds=%.6f expectedFiniteNonnegative=true"),
                    static_cast<double>(DeltaTime)));
            return;
        }
        if (DeltaTime > SustainedStressMaximumActiveDeltaSeconds)
        {
            const double RawDeltaMicrosecondsValue =
                std::ceil(static_cast<double>(DeltaTime) * 1'000'000.0);
            const uint64 RawDeltaMicroseconds =
                RawDeltaMicrosecondsValue >=
                        static_cast<double>(std::numeric_limits<uint64>::max())
                    ? std::numeric_limits<uint64>::max()
                    : static_cast<uint64>(RawDeltaMicrosecondsValue);
            const uint64 StableWallMicroseconds = static_cast<uint64>(
                std::floor(
                    SustainedStressStartupStableSeconds * 1'000'000.0));
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_STRESS_SUSTAINED_STABILIZATION_RESET] tick=0 rawDeltaUs=%llu stableFramesBeforeReset=%u stableWallUsBeforeReset=%llu maximumDeltaUs=%llu"),
                static_cast<unsigned long long>(RawDeltaMicroseconds),
                SustainedStressStartupStableFrames,
                static_cast<unsigned long long>(StableWallMicroseconds),
                static_cast<unsigned long long>(
                    SustainedStressMaximumActiveDeltaMicroseconds));
            FixedTimeAccumulator = 0.0;
            SustainedStressStartupStableFrames = 0;
            SustainedStressStartupStableSeconds = 0.0;
            return;
        }

        FixedTimeAccumulator = 0.0;
        ++SustainedStressStartupStableFrames;
        SustainedStressStartupStableSeconds +=
            static_cast<double>(DeltaTime);
        if (SustainedStressStartupStableFrames <
                SustainedStressStartupMinimumStableFrames ||
            SustainedStressStartupStableSeconds <
                SustainedStressStartupMinimumStableSeconds)
        {
            return;
        }

        const uint64 StableWallMicroseconds = static_cast<uint64>(
            std::floor(
                SustainedStressStartupStableSeconds * 1'000'000.0));
        SustainedStressReadyWallSeconds = FPlatformTime::Seconds();
        bSustainedStressTimingReady = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_STRESS_SUSTAINED_STABILIZED] tick=0 stableFrames=%u stableWallUs=%llu minimumStableFrames=%u minimumStableWallUs=1000000 maximumDeltaUs=%llu"),
            SustainedStressStartupStableFrames,
            static_cast<unsigned long long>(StableWallMicroseconds),
            SustainedStressStartupMinimumStableFrames,
            static_cast<unsigned long long>(
                SustainedStressMaximumActiveDeltaMicroseconds));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_STRESS_SUSTAINED_READY] fixture=Stress400Sustained tick=%llu checksum=%llu outcome=ongoing activePlayers=4 activeFactions=3 meridian=200 kharuun=100 hollowChoir=100 team0=100 team1=100 team2=100 team3=100 commandCores=4 combatUnits=396 ownedEntities=400 neutralWells=1 entities=401 views=401 tickRate=20 protectedCoreMask=15"),
            static_cast<unsigned long long>(Simulation->CurrentTick()),
            static_cast<unsigned long long>(Simulation->StateChecksum()));
        return;
    }

    if (bSustainedStressScenario &&
        (!FMath::IsFinite(DeltaTime) || DeltaTime < 0.0F ||
         DeltaTime > SustainedStressMaximumActiveDeltaSeconds))
    {
        FailSustainedStressContract(
            TEXT("SIM_TIME_CLAMP"),
            FString::Printf(
                TEXT("rawDeltaSeconds=%.6f permittedMaximum=0.250000"),
                static_cast<double>(DeltaTime)));
        return;
    }

    const double TickInterval =
        1.0 / static_cast<double>(Simulation->Config().ticksPerSecond);
    FixedTimeAccumulator += FMath::Min(static_cast<double>(DeltaTime), 0.25);

    int32 TicksThisFrame = 0;
    while (FixedTimeAccumulator >= TickInterval &&
           TicksThisFrame < MaximumCatchUpTicksPerFrame)
    {
        const int64 SustainedCombatHitPointsBeforeStep =
            bSustainedStressScenario
                ? GetSustainedStressCombatHitPoints()
                : 0;
        QueueOpponentCommands();
        Simulation->Step();
        if (bSustainedStressScenario &&
            !MaintainSustainedStressContractAfterFixedStep(
                SustainedCombatHitPointsBeforeStep))
        {
            FixedTimeAccumulator = 0.0;
            break;
        }
        FixedTimeAccumulator -= TickInterval;
        ++TicksThisFrame;
        if (bSustainedStressScenario &&
            Simulation->CurrentTick() % PrototypeTicksPerSecond == 0)
        {
            if (!SyncEntityViews(false) || !SyncTerrainView() || !SyncFogView())
            {
                FailSustainedStressContract(
                    TEXT("VIEW_SYNC_FAILED"),
                    TEXT("The exact heartbeat-boundary view sync failed."));
                FixedTimeAccumulator = 0.0;
                break;
            }
            if (!ValidateSustainedStressContract(true, true, true))
            {
                FixedTimeAccumulator = 0.0;
                break;
            }
        }
        AuditSeveralVoicesOneCommandContractAfterFixedStep();
        AuditBrokenSunContractAfterFixedStep();
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            const EEchoesSeveralVoicesOneCommandPhase Phase =
                GetSeveralVoicesOneCommandPhase();
            if (Phase == EEchoesSeveralVoicesOneCommandPhase::Complete ||
                Phase == EEchoesSeveralVoicesOneCommandPhase::Failed)
            {
                FixedTimeAccumulator = 0.0;
                break;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignTheBrokenSun)
        {
            const EEchoesBrokenSunPhase Phase = GetBrokenSunPhase();
            if (Phase == EEchoesBrokenSunPhase::Complete ||
                Phase == EEchoesBrokenSunPhase::Failed)
            {
                FixedTimeAccumulator = 0.0;
                break;
            }
        }
    }

    if (bSustainedStressFailed)
    {
        return;
    }

    if (FixedTimeAccumulator >= TickInterval)
    {
        FixedTimeAccumulator = FMath::Fmod(FixedTimeAccumulator, TickInterval);
        if (!bWarnedAboutTimeClamp)
        {
            UE_LOG(
                LogEchoes,
                Warning,
                TEXT("[ECHOES_SIM_TIME_CLAMP] Frame delay exceeded the fixed-step catch-up budget; excess wall time was discarded."));
            bWarnedAboutTimeClamp = true;
        }
        if (bSustainedStressScenario)
        {
            FailSustainedStressContract(
                TEXT("SIM_TIME_CLAMP"),
                TEXT("Fixed-step wall-time catch-up exceeded the permitted budget."));
            return;
        }
    }

    if (TicksThisFrame > 0)
    {
        if (!SyncEntityViews(false) || !SyncTerrainView() || !SyncFogView())
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_SIM_VIEW_SYNC_FAILED] A currently visible entity view could not be created; the prototype scenario was stopped."));
            if (bSustainedStressScenario)
            {
                FailSustainedStressContract(
                    TEXT("VIEW_SYNC_FAILED"),
                    TEXT("The post-frame authoritative view sync failed."));
                return;
            }
            if (AEchoesPlayerController* Controller =
                    Cast<AEchoesPlayerController>(GetWorld()->GetFirstPlayerController()))
            {
                Controller->NotifyRuntimeFailure(TEXT("ECHOES_VIEW_SYNC_FAILED"));
            }
            StopPrototypeScenario();
        }
        else
        {
            const echoes::sim::MatchOutcome Outcome = Simulation->Outcome();
            const EEchoesProloguePhase ProloguePhase = GetProloguePhase();
            const bool bPrologueFinished =
                ProloguePhase == EEchoesProloguePhase::Complete ||
                ProloguePhase == EEchoesProloguePhase::Failed;
            if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
                bPrologueFinished && !bMatchResultReported)
            {
                bMatchResultReported = true;
                bSimulationPaused = true;
                FutureWellChoice Consequence = FutureWellChoice::Dormant;
                for (const echoes::sim::Entity& Entity : Simulation->Entities())
                {
                    if (Entity.type == EntityType::FutureWell)
                    {
                        Consequence = Entity.wellChoice;
                        break;
                    }
                }
                FutureWellChoice RecordedConsequence = Consequence;
                FString CampaignFeedback;
                const EEchoesCampaignCommitStatus CampaignStatus =
                    ProloguePhase == EEchoesProloguePhase::Complete
                        ? CommitPrologueCompletion(
                              Consequence,
                              RecordedConsequence,
                              CampaignFeedback)
                        : EEchoesCampaignCommitStatus::NotApplicable;
                if (AEchoesPlayerController* Controller =
                        Cast<AEchoesPlayerController>(
                            GetWorld()->GetFirstPlayerController()))
                {
                    Controller->NotifyCampaignPrologueFinished(
                        ProloguePhase == EEchoesProloguePhase::Complete,
                        Consequence,
                        RecordedConsequence,
                        CampaignStatus);
                }
                const TCHAR* ResultName =
                    ProloguePhase == EEchoesProloguePhase::Complete
                        ? TEXT("success")
                        : TEXT("failure");
                const TCHAR* CampaignDetail = CampaignFeedback.IsEmpty()
                    ? TEXT("not-applicable")
                    : *CampaignFeedback;
                if (CampaignStatus ==
                    EEchoesCampaignCommitStatus::StorageFailure)
                {
                    UE_LOG(
                        LogEchoes,
                        Error,
                        TEXT("[ECHOES_PROLOGUE_FINISHED] result=%s phase=%s consequence=%u recordedConsequence=%u campaignStatus=%u tick=%llu detail=%s"),
                        ResultName,
                        FEchoesPrologueMissionModel::StableName(ProloguePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(Simulation->CurrentTick()),
                        CampaignDetail);
                }
                else
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_PROLOGUE_FINISHED] result=%s phase=%s consequence=%u recordedConsequence=%u campaignStatus=%u tick=%llu detail=%s"),
                        ResultName,
                        FEchoesPrologueMissionModel::StableName(ProloguePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(Simulation->CurrentTick()),
                        CampaignDetail);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignSevenAccounts &&
                     !bMatchResultReported)
            {
                const EEchoesSevenAccountsPhase SevenAccountsPhase =
                    GetSevenAccountsPhase();
                const bool bSevenAccountsFinished =
                    SevenAccountsPhase == EEchoesSevenAccountsPhase::Complete ||
                    SevenAccountsPhase == EEchoesSevenAccountsPhase::Failed;
                if (bSevenAccountsFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        SevenAccountsPhase == EEchoesSevenAccountsPhase::Complete
                            ? CommitSevenAccountsCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifySevenAccountsFinished(
                            SevenAccountsPhase ==
                                EEchoesSevenAccountsPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_SEVEN_ACCOUNTS_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        SevenAccountsPhase ==
                                EEchoesSevenAccountsPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesSevenAccountsMissionModel::StableName(
                            SevenAccountsPhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                    CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignCityReserve &&
                     !bMatchResultReported)
            {
                const EEchoesCityReservePhase CityReservePhase =
                    GetCityReservePhase();
                const bool bCityReserveFinished =
                    CityReservePhase == EEchoesCityReservePhase::Complete ||
                    CityReservePhase == EEchoesCityReservePhase::Failed;
                if (bCityReserveFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        CityReservePhase ==
                                EEchoesCityReservePhase::Complete
                            ? CommitCityReserveCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyCityReserveFinished(
                            CityReservePhase ==
                                EEchoesCityReservePhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_CITY_RESERVE_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        CityReservePhase ==
                                EEchoesCityReservePhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesCityReserveMissionModel::StableName(
                            CityReservePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignUnburiedRoad &&
                     !bMatchResultReported)
            {
                const EEchoesUnburiedRoadPhase UnburiedRoadPhase =
                    GetUnburiedRoadPhase();
                const bool bUnburiedRoadFinished =
                    UnburiedRoadPhase == EEchoesUnburiedRoadPhase::Complete ||
                    UnburiedRoadPhase == EEchoesUnburiedRoadPhase::Failed;
                if (bUnburiedRoadFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        UnburiedRoadPhase ==
                                EEchoesUnburiedRoadPhase::Complete
                            ? CommitUnburiedRoadCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyUnburiedRoadFinished(
                            UnburiedRoadPhase ==
                                EEchoesUnburiedRoadPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_UNBURIED_ROAD_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        UnburiedRoadPhase ==
                                EEchoesUnburiedRoadPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesUnburiedRoadMissionModel::StableName(
                            UnburiedRoadPhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignTermsOfContinuance &&
                     !bMatchResultReported)
            {
                const EEchoesTermsOfContinuancePhase ContinuancePhase =
                    GetTermsOfContinuancePhase();
                const bool bContinuanceFinished =
                    ContinuancePhase ==
                        EEchoesTermsOfContinuancePhase::Complete ||
                    ContinuancePhase ==
                        EEchoesTermsOfContinuancePhase::Failed;
                if (bContinuanceFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        ContinuancePhase ==
                                EEchoesTermsOfContinuancePhase::Complete
                            ? CommitTermsOfContinuanceCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyTermsOfContinuanceFinished(
                            ContinuancePhase ==
                                EEchoesTermsOfContinuancePhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_TERMS_OF_CONTINUANCE_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        ContinuancePhase ==
                                EEchoesTermsOfContinuancePhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesTermsOfContinuanceMissionModel::StableName(
                            ContinuancePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignNamesWithoutBirths &&
                     !bMatchResultReported)
            {
                const EEchoesNamesWithoutBirthsPhase NamesPhase =
                    GetNamesWithoutBirthsPhase();
                const bool bNamesFinished =
                    NamesPhase == EEchoesNamesWithoutBirthsPhase::Complete ||
                    NamesPhase == EEchoesNamesWithoutBirthsPhase::Failed;
                if (bNamesFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        NamesPhase ==
                                EEchoesNamesWithoutBirthsPhase::Complete
                            ? CommitNamesWithoutBirthsCompletion(
                                  RecordedConsequence, CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyNamesWithoutBirthsFinished(
                            NamesPhase ==
                                EEchoesNamesWithoutBirthsPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        NamesPhase ==
                                EEchoesNamesWithoutBirthsPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesNamesWithoutBirthsMissionModel::StableName(
                            NamesPhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignShapeOfSilence &&
                     !bMatchResultReported)
            {
                const EEchoesShapeOfSilencePhase ShapePhase =
                    GetShapeOfSilencePhase();
                const bool bShapeFinished =
                    ShapePhase == EEchoesShapeOfSilencePhase::Complete ||
                    ShapePhase == EEchoesShapeOfSilencePhase::Failed;
                if (bShapeFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        ShapePhase == EEchoesShapeOfSilencePhase::Complete
                            ? CommitShapeOfSilenceCompletion(
                                  RecordedConsequence, CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyShapeOfSilenceFinished(
                            ShapePhase ==
                                EEchoesShapeOfSilencePhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_SHAPE_OF_SILENCE_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        ShapePhase == EEchoesShapeOfSilencePhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesShapeOfSilenceMissionModel::StableName(
                            ShapePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignShapeBesideUs &&
                     !bMatchResultReported)
            {
                const EEchoesShapeBesideUsPhase BesidePhase =
                    GetShapeBesideUsPhase();
                const bool bBesideFinished =
                    BesidePhase == EEchoesShapeBesideUsPhase::Complete ||
                    BesidePhase == EEchoesShapeBesideUsPhase::Failed;
                if (bBesideFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        BesidePhase == EEchoesShapeBesideUsPhase::Complete
                            ? CommitShapeBesideUsCompletion(
                                  RecordedConsequence, CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyShapeBesideUsFinished(
                            BesidePhase ==
                                EEchoesShapeBesideUsPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_SHAPE_BESIDE_US_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s reciprocalContactOnly=true hollowChoirFactionImplemented=false"),
                        BesidePhase == EEchoesShapeBesideUsPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesShapeBesideUsMissionModel::StableName(
                            BesidePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignReserveAuthority &&
                     !bMatchResultReported)
            {
                const EEchoesReserveAuthorityPhase ReservePhase =
                    GetReserveAuthorityPhase();
                const bool bReserveFinished =
                    ReservePhase ==
                        EEchoesReserveAuthorityPhase::Complete ||
                    ReservePhase == EEchoesReserveAuthorityPhase::Failed;
                if (bReserveFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    const EEchoesCityDistrict DeferredDistrict =
                        GetReserveAuthorityDeferredDistrict();
                    EEchoesCityDistrict RecordedDeferredDistrict =
                        DeferredDistrict;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        ReservePhase ==
                                EEchoesReserveAuthorityPhase::Complete
                            ? CommitReserveAuthorityCompletion(
                                  RecordedConsequence,
                                  RecordedDeferredDistrict,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyReserveAuthorityFinished(
                            ReservePhase ==
                                EEchoesReserveAuthorityPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            DeferredDistrict,
                            RecordedDeferredDistrict,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_RESERVE_AUTHORITY_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u deferred=%u recordedDeferred=%u campaignStatus=%u tick=%llu detail=%s localDecisionOnly=true widerCityRestored=false civilianSurvivalUnmodeled=true"),
                        ReservePhase ==
                                EEchoesReserveAuthorityPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesReserveAuthorityMissionModel::StableName(
                            ReservePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(DeferredDistrict),
                        static_cast<uint8>(RecordedDeferredDistrict),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignChoirAtLumeReach &&
                     !bMatchResultReported)
            {
                const EEchoesChoirAtLumeReachPhase ChoirPhase =
                    GetChoirAtLumeReachPhase();
                const bool bChoirFinished =
                    ChoirPhase == EEchoesChoirAtLumeReachPhase::Complete ||
                    ChoirPhase == EEchoesChoirAtLumeReachPhase::Failed;
                if (bChoirFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const echoes::sim::Entity* Well =
                        Simulation->FindEntity(ChoirAtLumeReachWellId);
                    const FutureWellChoice Consequence =
                        Well != nullptr
                            ? Well->wellChoice
                            : FutureWellChoice::Dormant;
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        ChoirPhase ==
                                EEchoesChoirAtLumeReachPhase::Complete
                            ? CommitChoirAtLumeReachCompletion(
                                  Consequence,
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyChoirAtLumeReachFinished(
                            ChoirPhase ==
                                EEchoesChoirAtLumeReachPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    const FEchoesChoirAtLumeReachPlan Plan =
                        GetChoirAtLumeReachPlan();
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_CHOIR_AT_LUME_REACH_FINISHED] result=%s phase=%s priorBranch=%u newWellChoice=%u recordedWellChoice=%u deferredDistrict=%u campaignStatus=%u tick=%llu detail=%s maraPresence=liaisonOnly choirPresence=nonPlayablePublicContact mixedFactionCommand=false hiddenAttribution=false causationClaim=false"),
                        ChoirPhase ==
                                EEchoesChoirAtLumeReachPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesChoirAtLumeReachMissionModel::StableName(
                            ChoirPhase),
                        static_cast<uint8>(Plan.PriorChoice),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(Plan.DeferredDistrict),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignNoNeutralLedger &&
                     !bMatchResultReported)
            {
                const EEchoesNoNeutralLedgerPhase AlliancePhase =
                    GetNoNeutralLedgerPhase();
                const bool bAllianceFinished =
                    AlliancePhase == EEchoesNoNeutralLedgerPhase::Complete ||
                    AlliancePhase == EEchoesNoNeutralLedgerPhase::Failed;
                if (bAllianceFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FEchoesNoNeutralLedgerPlan Plan =
                        GetNoNeutralLedgerPlan();
                    FutureWellChoice RecordedProtocol = Plan.LumeProtocol;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        AlliancePhase ==
                                EEchoesNoNeutralLedgerPhase::Complete
                            ? CommitNoNeutralLedgerCompletion(
                                  RecordedProtocol,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyNoNeutralLedgerFinished(
                            AlliancePhase ==
                                EEchoesNoNeutralLedgerPhase::Complete,
                            Plan.LumeProtocol,
                            RecordedProtocol,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_NO_NEUTRAL_LEDGER_FINISHED] result=%s phase=%s planKey=%u founding=%u districtA=%u districtB=%u deferred=%u lumeProtocol=%u recordedProtocol=%u campaignStatus=%u tick=%llu detail=%s inheritedRecords=10 localAuthority=KharuunAssemblies meridianPresence=neutralPoweredPublicInterfacesOnly kharuunEvidenceInterface=neutralPublic choirPresence=nonPlayablePublicContact mixedFactionCommand=false hiddenTrust=false survivorVarianceUnmodeled=true proxyAttribution=false"),
                        AlliancePhase ==
                                EEchoesNoNeutralLedgerPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesNoNeutralLedgerMissionModel::StableName(
                            AlliancePhase),
                        Plan.StablePlanKey,
                        static_cast<uint8>(Plan.FoundingDoctrine),
                        static_cast<uint8>(
                            Plan.FirstContributingDistrict),
                        static_cast<uint8>(
                            Plan.SecondContributingDistrict),
                        static_cast<uint8>(Plan.DeferredDistrict),
                        static_cast<uint8>(Plan.LumeProtocol),
                        static_cast<uint8>(RecordedProtocol),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignFutureThatWon &&
                     !bMatchResultReported)
            {
                const EEchoesFutureThatWonPhase RestorationPhase =
                    GetFutureThatWonPhase();
                const bool bRestorationFinished =
                    RestorationPhase ==
                        EEchoesFutureThatWonPhase::Complete ||
                    RestorationPhase == EEchoesFutureThatWonPhase::Failed;
                if (bRestorationFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FEchoesFutureThatWonPlan Plan =
                        GetFutureThatWonPlan();
                    FutureWellChoice RecordedProtocol =
                        Plan.RecordedProtocol;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        RestorationPhase ==
                                EEchoesFutureThatWonPhase::Complete
                            ? CommitFutureThatWonCompletion(
                                  RecordedProtocol,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyFutureThatWonFinished(
                            RestorationPhase ==
                                EEchoesFutureThatWonPhase::Complete,
                            Plan.RecordedProtocol,
                            RecordedProtocol,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_FUTURE_THAT_WON_FINISHED] result=%s phase=%s planKey=%u founding=%u districtA=%u districtB=%u deferred=%u protocol=%u recordedProtocol=%u campaignStatus=%u tick=%llu detail=%s stabilityTicks=%llu inheritedRecords=11 localAuthority=KharuunAssemblies rhysePresence=attributablePublicApparatusOnly mixedFactionCommand=false civilianCountsUnmodeled=true populationRestorationUnproven=true permanentFutureUnproven=true ethicalJustificationUnproven=true"),
                        RestorationPhase ==
                                EEchoesFutureThatWonPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesFutureThatWonMissionModel::StableName(
                            RestorationPhase),
                        Plan.StablePlanKey,
                        static_cast<uint8>(Plan.FoundingDoctrine),
                        static_cast<uint8>(
                            Plan.FirstContributingDistrict),
                        static_cast<uint8>(
                            Plan.SecondContributingDistrict),
                        static_cast<uint8>(Plan.DeferredDistrict),
                        static_cast<uint8>(Plan.RecordedProtocol),
                        static_cast<uint8>(RecordedProtocol),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback,
                        static_cast<unsigned long long>(
                            Plan.StabilityWindowTicks));
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignTheBrokenSun &&
                     !bMatchResultReported)
            {
                const EEchoesBrokenSunPhase BrokenSunPhase =
                    GetBrokenSunPhase();
                const bool bBrokenSunFinished =
                    BrokenSunPhase == EEchoesBrokenSunPhase::Complete ||
                    BrokenSunPhase == EEchoesBrokenSunPhase::Failed;
                if (bBrokenSunFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
                    EEchoesFinalResolution RecordedResolution =
                        SelectedBrokenSunResolution;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        BrokenSunPhase == EEchoesBrokenSunPhase::Complete
                            ? CommitBrokenSunCompletion(
                                  RecordedResolution,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyBrokenSunFinished(
                            BrokenSunPhase ==
                                EEchoesBrokenSunPhase::Complete,
                            SelectedBrokenSunResolution,
                            RecordedResolution,
                            CampaignStatus);
                    }
                    const uint64 RequiredResolutionTicks =
                        FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
                            Plan,
                            SelectedBrokenSunResolution);
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_BROKEN_SUN_FINISHED] result=%s phase=%s planKey=%u founding=%u protocol=%u resolution=%s recordedResolution=%s availability=0x%02X campaignStatus=%u tick=%llu detail=%s inheritedRecords=14 localAuthority=HollowChoir mixedFactionCommand=false approach=%u conduit=%u holdTicks=%llu endingLedgerEstablished=%s namedWitnessesModeled=true broadPoliticalAcceptanceUnproven=true populationOutcomeUnmodeled=true permanentFutureUnproven=true campaignBalanceUnproven=true ordinaryHumanCompletionUnproven=true releaseReadinessUnproven=true"),
                        BrokenSunPhase == EEchoesBrokenSunPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesBrokenSunMissionModel::StableName(
                            BrokenSunPhase),
                        Plan.StablePlanKey,
                        static_cast<uint8>(Plan.FoundingDoctrine),
                        static_cast<uint8>(Plan.RecordedProtocol),
                        FEchoesBrokenSunMissionModel::ResolutionStableName(
                            SelectedBrokenSunResolution),
                        FEchoesBrokenSunMissionModel::ResolutionStableName(
                            RecordedResolution),
                        Plan.AvailableFinalResolutions,
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback,
                        BrokenSunApproachAnchorId,
                        BrokenSunResolutionConduitId,
                        static_cast<unsigned long long>(
                            RequiredResolutionTicks),
                        CampaignStatus ==
                                    EEchoesCampaignCommitStatus::Added ||
                                CampaignStatus ==
                                    EEchoesCampaignCommitStatus::AlreadyRecorded
                            ? TEXT("true")
                            : TEXT("false"));
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
                     !bMatchResultReported)
            {
                const EEchoesSeveralVoicesOneCommandPhase VoicesPhase =
                    GetSeveralVoicesOneCommandPhase();
                const bool bVoicesFinished =
                    VoicesPhase ==
                        EEchoesSeveralVoicesOneCommandPhase::Complete ||
                    VoicesPhase ==
                        EEchoesSeveralVoicesOneCommandPhase::Failed;
                if (bVoicesFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FEchoesSeveralVoicesOneCommandPlan Plan =
                        GetSeveralVoicesOneCommandPlan();
                    FutureWellChoice RecordedProtocol =
                        Plan.RecordedProtocol;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        VoicesPhase ==
                                EEchoesSeveralVoicesOneCommandPhase::Complete
                            ? CommitSeveralVoicesOneCommandCompletion(
                                  RecordedProtocol,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifySeveralVoicesOneCommandFinished(
                            VoicesPhase ==
                                EEchoesSeveralVoicesOneCommandPhase::Complete,
                            Plan.RecordedProtocol,
                            RecordedProtocol,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_SEVERAL_VOICES_ONE_COMMAND_FINISHED] result=%s phase=%s planKey=%u founding=%u protocol=%u recordedProtocol=%u campaignStatus=%u tick=%llu detail=%s inheritedRecords=13 localAuthority=HollowChoir possibleVoice=%u manifestVoice=%u neme=%u identityResolveTicks=160 crisisHoldTicks=%llu factionPerspectiveOperation=true finalChoirFateDecided=false broadCampaignBalanceUnproven=true ordinaryHumanCompletionUnproven=true"),
                        VoicesPhase ==
                                EEchoesSeveralVoicesOneCommandPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesSeveralVoicesOneCommandMissionModel::StableName(
                            VoicesPhase),
                        Plan.StablePlanKey,
                        static_cast<uint8>(Plan.FoundingDoctrine),
                        static_cast<uint8>(Plan.RecordedProtocol),
                        static_cast<uint8>(RecordedProtocol),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback,
                        SeveralVoicesPossibleVoiceId,
                        SeveralVoicesManifestVoiceId,
                        SeveralVoicesNemeId,
                        static_cast<unsigned long long>(
                            SeveralVoicesCrisisHoldTicks));
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
                     !bMatchResultReported)
            {
                const EEchoesAssemblyOfTheMissingPhase AssemblyPhase =
                    GetAssemblyOfTheMissingPhase();
                const bool bAssemblyFinished =
                    AssemblyPhase ==
                        EEchoesAssemblyOfTheMissingPhase::Complete ||
                    AssemblyPhase ==
                        EEchoesAssemblyOfTheMissingPhase::Failed;
                if (bAssemblyFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FEchoesAssemblyOfTheMissingPlan Plan =
                        GetAssemblyOfTheMissingPlan();
                    FutureWellChoice RecordedProtocol =
                        Plan.RecordedProtocol;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        AssemblyPhase ==
                                EEchoesAssemblyOfTheMissingPhase::Complete
                            ? CommitAssemblyOfTheMissingCompletion(
                                  RecordedProtocol,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyAssemblyOfTheMissingFinished(
                            AssemblyPhase ==
                                EEchoesAssemblyOfTheMissingPhase::Complete,
                            Plan.RecordedProtocol,
                            RecordedProtocol,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_ASSEMBLY_OF_THE_MISSING_FINISHED] result=%s phase=%s planKey=%u founding=%u districtA=%u districtB=%u deferred=%u protocol=%u recordedProtocol=%u campaignStatus=%u tick=%llu detail=%s inheritedRecords=12 localAuthority=KharuunAssemblies publicRecordAssemblyOnly=true mixedFactionCommand=false responsibilityUnassigned=true hiddenAuthorshipUnproven=true trustUnproven=true consentUnproven=true civilianStateUnmodeled=true cryptographicAuthenticityUnproven=true"),
                        AssemblyPhase ==
                                EEchoesAssemblyOfTheMissingPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesAssemblyOfTheMissingMissionModel::StableName(
                            AssemblyPhase),
                        Plan.StablePlanKey,
                        static_cast<uint8>(Plan.FoundingDoctrine),
                        static_cast<uint8>(
                            Plan.FirstContributingDistrict),
                        static_cast<uint8>(
                            Plan.SecondContributingDistrict),
                        static_cast<uint8>(Plan.DeferredDistrict),
                        static_cast<uint8>(Plan.RecordedProtocol),
                        static_cast<uint8>(RecordedProtocol),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation == EEchoesOperationMode::Skirmish &&
                     Outcome != echoes::sim::MatchOutcome::Ongoing &&
                     !bMatchResultReported)
            {
                bMatchResultReported = true;
                if (AEchoesPlayerController* Controller =
                        Cast<AEchoesPlayerController>(
                            GetWorld()->GetFirstPlayerController()))
                {
                    Controller->NotifyMatchFinished(Outcome);
                }
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_MATCH_FINISHED] outcome=%u tick=%llu"),
                    static_cast<uint8>(Outcome),
                    static_cast<unsigned long long>(Simulation->CurrentTick()));
            }
            AdvancePrologueCompletionPresentation();
            if (!bLoggedFirstTick)
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_SIM_FIRST_TICK] tick=%llu checksum=%llu visibleViews=%d."),
                    static_cast<unsigned long long>(Simulation->CurrentTick()),
                    static_cast<unsigned long long>(Simulation->StateChecksum()),
                    EntityViews.Num());
                bLoggedFirstTick = true;
            }
            if (bResearchPresentationScenario ||
                bResearchInterruptionPresentationScenario)
            {
                const echoes::sim::PlayerState* Player =
                    Simulation->FindPlayer(LocalPlayerId);
                if (Player != nullptr &&
                    !bLoggedResearchPresentationActive &&
                    Player->activeResearch == ResearchPresentationTechnology)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_ACTIVE] technology=%s progress=%d required=%d controlled=true"),
                        ResearchStableName(ResearchPresentationTechnology),
                        Player->researchProgress,
                        Player->researchRequired);
                    bLoggedResearchPresentationActive = true;
                }
                if (Player != nullptr &&
                    !bLoggedResearchPresentationComplete &&
                    bResearchPresentationScenario &&
                    Player->HasCompletedResearch(
                        ResearchPresentationTechnology))
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_COMPLETE] technology=%s completed=true controlled=true"),
                        ResearchStableName(ResearchPresentationTechnology));
                    bLoggedResearchPresentationComplete = true;
                }
                if (Player != nullptr &&
                    !bLoggedResearchPresentationInterrupted &&
                    bResearchInterruptionPresentationScenario &&
                    Player->lastInterruptedResearch ==
                        ResearchPresentationTechnology)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_INTERRUPTED] technology=%s producerDestroyed=true costsRefunded=false controlled=true"),
                        ResearchStableName(ResearchPresentationTechnology));
                    bLoggedResearchPresentationInterrupted = true;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        if (!Controller->IsTechnologyPanelVisible())
                        {
                            Controller->ToggleTechnologyPanel();
                        }
                        if (Controller->IsTechnologyPanelVisible())
                        {
                            UE_LOG(
                                LogEchoes,
                                Display,
                                TEXT("[ECHOES_RESEARCH_INTERRUPTION_PANEL_READY] visible=true paused=true controlled=true release=false"));
                        }
                    }
                }
            }
            if (bKharuunSystemsPresentationScenario &&
                !bLoggedKharuunSystemsPresentation)
            {
                bool bWaystoneTransitioning = false;
                bool bCarapaceMoltActive = false;
                bool bMineralCoverActive = false;
                for (const echoes::sim::Entity& Entity : Simulation->Entities())
                {
                    if (Entity.owner == LocalPlayerId &&
                        Entity.type == EntityType::Dropoff &&
                        Entity.waystoneMode ==
                            echoes::sim::WaystoneMode::Uprooting)
                    {
                        bWaystoneTransitioning = true;
                    }
                    if (Entity.owner == LocalPlayerId &&
                        Entity.pendingWarformAdaptation ==
                            echoes::sim::WarformAdaptation::Carapace)
                    {
                        bCarapaceMoltActive = true;
                    }
                    if (Entity.owner == LocalPlayerId &&
                        Entity.temporaryMineralCover && Entity.hitPoints > 0)
                    {
                        bMineralCoverActive = true;
                    }
                }
                const std::optional<echoes::sim::PlayerView> LocalView =
                    Simulation->CreatePlayerView(LocalPlayerId);
                const int32 VibrationContacts =
                    LocalView.has_value()
                        ? static_cast<int32>(
                              LocalView->VibrationSignatures().size())
                        : 0;
                const bool bHiddenSourceDisclosed =
                    LocalView.has_value() &&
                    std::any_of(
                        LocalView->Entities().begin(),
                        LocalView->Entities().end(),
                        [](const echoes::sim::Entity& Entity)
                        {
                            return Entity.owner == 2;
                        });
                if (bWaystoneTransitioning && bCarapaceMoltActive &&
                    bMineralCoverActive && VibrationContacts > 0 &&
                    !bHiddenSourceDisclosed)
                {
                    bLoggedKharuunSystemsPresentation = true;
                    bSimulationPaused = true;
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_ACTIVE] waystone=uprooting warform=carapace_molt cover=active vibrationContacts=%d anonymous=true hiddenSourceDisclosed=false paused=true controlled=true release=false"),
                        VibrationContacts);
                }
            }
            if (bStressScenario && !bSustainedStressScenario &&
                !bLoggedStressCombat &&
                Simulation->CurrentTick() >= 20)
            {
                int32 RemainingSoldiers = 0;
                int32 DamagedSoldiers = 0;
                for (const echoes::sim::Entity& Entity : Simulation->Entities())
                {
                    if (Entity.type != EntityType::Soldier &&
                        Entity.type != EntityType::HeavyUnit &&
                        Entity.type != EntityType::ScoutUnit)
                    {
                        continue;
                    }
                    ++RemainingSoldiers;
                    DamagedSoldiers += Entity.hitPoints < Entity.maxHitPoints ? 1 : 0;
                }
                const int32 DestroyedSoldiers = 396 - RemainingSoldiers;
                if (DamagedSoldiers > 0 || DestroyedSoldiers > 0)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_STRESS_COMBAT_ACTIVE] tick=%llu damaged=%d destroyed=%d remaining=%d visibleViews=%d"),
                        static_cast<unsigned long long>(Simulation->CurrentTick()),
                        DamagedSoldiers,
                        DestroyedSoldiers,
                        RemainingSoldiers,
                        EntityViews.Num());
                    bLoggedStressCombat = true;
                }
            }
        }
    }
}

void UEchoesSimulationSubsystem::QueueOpponentCommands()
{
    if (bStressScenario || bNetworkHumanOpponent ||
        bPointerCombatGuardPresentationScenario ||
        !Simulation.IsValid() ||
        Simulation->CurrentTick() % Simulation->Config().ticksPerSecond != 0)
    {
        return;
    }

    const std::optional<echoes::sim::PlayerView> PlayerView =
        Simulation->CreatePlayerView(OpponentPlayerId);
    if (!PlayerView.has_value())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_AI_PLAYER_VIEW_FAILED] player=%u"),
            OpponentPlayerId);
        return;
    }
    if (!bLoggedAiPlayerView)
    {
        int32 OwnedEntities = 0;
        for (const echoes::sim::Entity& Entity : PlayerView->Entities())
        {
            OwnedEntities += Entity.owner == OpponentPlayerId ? 1 : 0;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_AI_PLAYER_VIEW] player=%u owned=%d observed=%d hiddenEntitiesExcluded=true opponentInternalsRedacted=true authoritativeWorldHandle=false"),
            OpponentPlayerId,
            OwnedEntities,
            static_cast<int32>(PlayerView->Entities().size()));
        bLoggedAiPlayerView = true;
    }
    const std::vector<echoes::sim::Command> Commands =
        echoes::sim::Simulation::GenerateAiCommands(
            *PlayerView,
            echoes::sim::AiPersonality::Adaptive);
    for (const echoes::sim::Command& Command : Commands)
    {
        std::string Rejection;
        if (Simulation->QueueCommand(Command, &Rejection))
        {
            if (!bLoggedAiExpansion &&
                Command.type == echoes::sim::CommandType::Build)
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_EXPANSION] personality=adaptive actor=%u buildType=%u tile=(%d,%d) visibilityBounded=true"),
                    Command.actor,
                    static_cast<uint8>(Command.buildType),
                    Command.position.x.FloorToInt(),
                    Command.position.y.FloorToInt());
                bLoggedAiExpansion = true;
            }
            if (!bLoggedAiRetreat &&
                (Command.type == echoes::sim::CommandType::Move ||
                 Command.type == echoes::sim::CommandType::Hold))
            {
                const echoes::sim::Entity* Actor =
                    Simulation->FindEntity(Command.actor);
                if (Actor != nullptr &&
                    (Actor->type == echoes::sim::EntityType::Soldier ||
                     Actor->type == echoes::sim::EntityType::HeavyUnit ||
                     Actor->type == echoes::sim::EntityType::ScoutUnit) &&
                    Actor->maxHitPoints > 0 &&
                    static_cast<int64>(Actor->hitPoints) * 100 <=
                        static_cast<int64>(Actor->maxHitPoints) * 35)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_AI_RETREAT] personality=adaptive actor=%u health=%d/%d action=%s visibilityBounded=true"),
                        Command.actor,
                        Actor->hitPoints,
                        Actor->maxHitPoints,
                        Command.type == echoes::sim::CommandType::Hold
                            ? TEXT("hold-near-core")
                            : TEXT("withdraw-to-core"));
                    bLoggedAiRetreat = true;
                }
            }
            if (!bLoggedAiAdaptation &&
                Command.type == echoes::sim::CommandType::AdaptWarform)
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_ADAPTATION] personality=adaptive actor=%u site=%u form=%u compositionVisible=true"),
                    Command.actor,
                    Command.target,
                    static_cast<uint8>(Command.warformAdaptation));
                bLoggedAiAdaptation = true;
            }
            if (!bLoggedAiMineralCover &&
                Command.type == echoes::sim::CommandType::RaiseMineralCover)
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_MINERAL_COVER] personality=adaptive actor=%u tile=(%d,%d) visibilityBounded=true"),
                    Command.actor,
                    Command.position.x.FloorToInt(),
                    Command.position.y.FloorToInt());
                bLoggedAiMineralCover = true;
            }
            if (!bLoggedAiVibrationResponse &&
                Command.type == echoes::sim::CommandType::AttackMove &&
                !PlayerView->VibrationSignatures().empty())
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_VIBRATION_RESPONSE] personality=adaptive actor=%u tile=(%d,%d) anonymous=true visibilityBounded=true"),
                    Command.actor,
                    Command.position.x.FloorToInt(),
                    Command.position.y.FloorToInt());
                bLoggedAiVibrationResponse = true;
            }
        }
        else
        {
            UE_LOG(
                LogEchoes,
                Warning,
                TEXT("[ECHOES_AI_COMMAND_REJECTED] actor=%u reason=%s"),
                Command.actor,
                UTF8_TO_TCHAR(Rejection.c_str()));
        }
    }
}

void UEchoesSimulationSubsystem::
AuditSeveralVoicesOneCommandContractAfterFixedStep()
{
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand ||
        !bScenarioReady || !Simulation.IsValid() ||
        bSeveralVoicesCrisisContractFailed)
    {
        return;
    }

    const FEchoesSeveralVoicesOneCommandPlan Plan =
        GetSeveralVoicesOneCommandPlan();
    const echoes::sim::Entity* PossibleVoice =
        Simulation->FindEntity(SeveralVoicesPossibleVoiceId);
    const echoes::sim::Entity* ManifestVoice =
        Simulation->FindEntity(SeveralVoicesManifestVoiceId);
    const echoes::sim::Entity* Neme =
        Simulation->FindEntity(SeveralVoicesNemeId);
    const echoes::sim::Entity* ResearchLoom =
        Simulation->FindEntity(SeveralVoicesResearchLoomId);
    const auto IsProtectedChoirEntity = [](
        const echoes::sim::Entity* Entity,
        EntityType Type)
    {
        return Entity != nullptr && Entity->owner == LocalPlayerId &&
               Entity->faction == Faction::HollowChoir &&
               Entity->type == Type && Entity->hitPoints > 0 &&
               Entity->completed;
    };

    bool bLocalCoreIntact = false;
    bool bPhaseAnchorComplete = false;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner != LocalPlayerId ||
            Entity.faction != Faction::HollowChoir ||
            Entity.hitPoints <= 0 || !Entity.completed)
        {
            continue;
        }
        bLocalCoreIntact |= Entity.type == EntityType::CommandCore;
        bPhaseAnchorComplete |=
            Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                Plan.CrisisAnchorSite,
                SeveralVoicesOneCommandSiteRadiusTiles);
    }

    if (bPhaseAnchorComplete && !bSeveralVoicesCrisisHoldStarted)
    {
        bSeveralVoicesCrisisHoldStarted = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_SEVERAL_VOICES_CRISIS_HOLD_STARTED] tick=%llu"),
            static_cast<unsigned long long>(
                Simulation->CurrentTick()));
    }
    if (!bSeveralVoicesCrisisHoldStarted)
    {
        return;
    }

    const echoes::sim::PlayerState* Player =
        Simulation->FindPlayer(LocalPlayerId);
    const bool bPossibleContractValid =
        IsProtectedChoirEntity(PossibleVoice, EntityType::Soldier) &&
        PossibleVoice->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Possible &&
        IsWithinTiles(
            PossibleVoice->position,
            Plan.PossibleVoiceSite,
            SeveralVoicesOneCommandSiteRadiusTiles);
    const bool bManifestContractValid =
        IsProtectedChoirEntity(ManifestVoice, EntityType::HeavyUnit) &&
        ManifestVoice->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Manifest &&
        IsWithinTiles(
            ManifestVoice->position,
            Plan.ManifestVoiceSite,
            SeveralVoicesOneCommandSiteRadiusTiles);
    const bool bNemeContractValid =
        IsProtectedChoirEntity(Neme, EntityType::ScoutUnit) &&
        Neme->choirIdentityState !=
            echoes::sim::ChoirIdentityState::NotChoir &&
        IsWithinTiles(
            Neme->position,
            Plan.NemeCommandSite,
            SeveralVoicesOneCommandSiteRadiusTiles);
    const bool bResearchContractValid =
        IsProtectedChoirEntity(ResearchLoom, EntityType::Barracks) &&
        Player != nullptr &&
        Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirHeldAlternatives) &&
        Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirSharedResolution);
    if (!bLocalCoreIntact || !bPhaseAnchorComplete ||
        !bPossibleContractValid || !bManifestContractValid ||
        !bNemeContractValid || !bResearchContractValid ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        bSeveralVoicesCrisisContractFailed = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_SEVERAL_VOICES_CRISIS_CONTRACT_FAILED] tick=%llu core=%s anchor=%s possible=%s manifest=%s neme=%s research=%s outcome=%u irreversible=true"),
            static_cast<unsigned long long>(
                Simulation->CurrentTick()),
            bLocalCoreIntact ? TEXT("true") : TEXT("false"),
            bPhaseAnchorComplete ? TEXT("true") : TEXT("false"),
            bPossibleContractValid ? TEXT("true") : TEXT("false"),
            bManifestContractValid ? TEXT("true") : TEXT("false"),
            bNemeContractValid ? TEXT("true") : TEXT("false"),
            bResearchContractValid ? TEXT("true") : TEXT("false"),
            static_cast<uint8>(Simulation->Outcome()));
    }
}

void UEchoesSimulationSubsystem::AuditBrokenSunContractAfterFixedStep()
{
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignTheBrokenSun ||
        !bScenarioReady || !Simulation.IsValid() ||
        bBrokenSunResolutionContractFailed)
    {
        return;
    }

    const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
    const echoes::sim::Vec2 ResolutionSite =
        FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
            Plan,
            SelectedBrokenSunResolution);
    const uint64 RequiredResolutionTicks =
        FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
            Plan,
            SelectedBrokenSunResolution);
    const auto IsEntity = [](
        const echoes::sim::Entity* Entity,
        uint8 Owner,
        Faction EntityFaction,
        EntityType Type)
    {
        return Entity != nullptr && Entity->owner == Owner &&
               Entity->faction == EntityFaction && Entity->type == Type &&
               Entity->hitPoints > 0 && Entity->completed;
    };
    const auto IsLocalChoir = [&IsEntity](
        const echoes::sim::Entity* Entity,
        EntityType Type)
    {
        return IsEntity(
            Entity,
            UEchoesSimulationSubsystem::LocalPlayerId,
            Faction::HollowChoir,
            Type);
    };

    const echoes::sim::Entity* Voice =
        Simulation->FindEntity(BrokenSunAccordVoiceId);
    const echoes::sim::Entity* Heavy =
        Simulation->FindEntity(BrokenSunAccordHeavyId);
    const echoes::sim::Entity* Neme =
        Simulation->FindEntity(BrokenSunNemeId);
    const echoes::sim::Entity* Worker =
        Simulation->FindEntity(BrokenSunWorkerId);
    const echoes::sim::Entity* Mara =
        Simulation->FindEntity(BrokenSunMaraId);
    const echoes::sim::Entity* Oruun =
        Simulation->FindEntity(BrokenSunOruunId);
    const echoes::sim::Entity* Talar =
        Simulation->FindEntity(BrokenSunTalarId);
    const echoes::sim::PlayerState* Player =
        Simulation->FindPlayer(LocalPlayerId);

    bool bLocalCoreIntact = false;
    const echoes::sim::Entity* ApproachCandidate = nullptr;
    const echoes::sim::Entity* ConduitCandidate = nullptr;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner != LocalPlayerId ||
            Entity.faction != Faction::HollowChoir ||
            Entity.hitPoints <= 0 || !Entity.completed)
        {
            continue;
        }
        bLocalCoreIntact |= Entity.type == EntityType::CommandCore;
        if (Entity.type != EntityType::UtilityStructure)
        {
            continue;
        }
        if (IsWithinTiles(
                Entity.position,
                Plan.CrownfallApproachSite,
                BrokenSunSiteRadiusTiles) &&
            (ApproachCandidate == nullptr ||
             Entity.id < ApproachCandidate->id))
        {
            ApproachCandidate = &Entity;
        }
        if (SelectedBrokenSunResolution !=
                EEchoesFinalResolution::None &&
            IsWithinTiles(
                Entity.position,
                ResolutionSite,
                BrokenSunConvergenceRadiusTiles) &&
            (ConduitCandidate == nullptr ||
             Entity.id < ConduitCandidate->id))
        {
            ConduitCandidate = &Entity;
        }
    }

    if (BrokenSunApproachAnchorId == 0 && ApproachCandidate != nullptr)
    {
        BrokenSunApproachAnchorId = ApproachCandidate->id;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_BROKEN_SUN_APPROACH_BOUND] tick=%llu anchor=%u exactObjective=true"),
            static_cast<unsigned long long>(Simulation->CurrentTick()),
            BrokenSunApproachAnchorId);
    }
    const echoes::sim::Entity* ApproachAnchor =
        Simulation->FindEntity(BrokenSunApproachAnchorId);
    const bool bApproachValid =
        IsLocalChoir(ApproachAnchor, EntityType::UtilityStructure) &&
        IsWithinTiles(
            ApproachAnchor->position,
            Plan.CrownfallApproachSite,
            BrokenSunSiteRadiusTiles);

    const bool bVoiceValid =
        IsLocalChoir(Voice, EntityType::Soldier) &&
        Voice->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Possible &&
        IsWithinTiles(
            Voice->position,
            Plan.MaraAccordSite,
            BrokenSunSiteRadiusTiles);
    const bool bHeavyValid =
        IsLocalChoir(Heavy, EntityType::HeavyUnit) &&
        Heavy->choirIdentityState ==
            echoes::sim::ChoirIdentityState::Manifest &&
        IsWithinTiles(
            Heavy->position,
            Plan.OruunAccordSite,
            BrokenSunSiteRadiusTiles);
    const bool bNemeValid =
        IsLocalChoir(Neme, EntityType::ScoutUnit) &&
        Neme->choirIdentityState !=
            echoes::sim::ChoirIdentityState::NotChoir &&
        IsWithinTiles(
            Neme->position,
            Plan.NemeAccordSite,
            BrokenSunSiteRadiusTiles);
    const bool bWorkerValid =
        IsLocalChoir(Worker, EntityType::Worker);
    const bool bMaraValid =
        IsEntity(
            Mara,
            2,
            Faction::MeridianCompact,
            EntityType::ScoutUnit) &&
        IsWithinTiles(
            Mara->position,
            Plan.MaraAccordSite,
            BrokenSunSiteRadiusTiles);
    const bool bOruunValid =
        IsEntity(
            Oruun,
            3,
            Faction::KharuunAssemblies,
            EntityType::ScoutUnit) &&
        IsWithinTiles(
            Oruun->position,
            Plan.OruunAccordSite,
            BrokenSunSiteRadiusTiles);
    const bool bTalarValid =
        IsEntity(
            Talar,
            2,
            Faction::MeridianCompact,
            EntityType::Worker) &&
        IsWithinTiles(
            Talar->position,
            Plan.TalarPublicRecordSite,
            BrokenSunSiteRadiusTiles);
    const bool bResearchValid =
        Player != nullptr &&
        Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirHeldAlternatives) &&
        Player->HasCompletedResearch(
            echoes::sim::ResearchType::ChoirSharedResolution);
    const bool bAccordValid =
        bVoiceValid && bHeavyValid && bNemeValid && bResearchValid;
    const bool bResolutionValid =
        SelectedBrokenSunResolution == EEchoesFinalResolution::None ||
        FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            Plan,
            SelectedBrokenSunResolution);

    if (SelectedBrokenSunResolution != EEchoesFinalResolution::None &&
        BrokenSunResolutionConduitId == 0 &&
        ConduitCandidate != nullptr && bApproachValid && bAccordValid &&
        bWorkerValid && bMaraValid && bOruunValid && bTalarValid &&
        bLocalCoreIntact && bResolutionValid)
    {
        BrokenSunResolutionConduitId = ConduitCandidate->id;
    }
    const echoes::sim::Entity* ResolutionConduit =
        Simulation->FindEntity(BrokenSunResolutionConduitId);
    const bool bConduitValid =
        IsLocalChoir(ResolutionConduit, EntityType::UtilityStructure) &&
        IsWithinTiles(
            ResolutionConduit->position,
            ResolutionSite,
            BrokenSunConvergenceRadiusTiles);

    if (!bBrokenSunResolutionHoldStarted && bConduitValid &&
        SelectedBrokenSunResolution != EEchoesFinalResolution::None &&
        bApproachValid && bAccordValid && bWorkerValid && bMaraValid &&
        bOruunValid && bTalarValid && bLocalCoreIntact &&
        bResolutionValid &&
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing)
    {
        bBrokenSunResolutionHoldStarted = true;
        BrokenSunResolutionStartTick = Simulation->CurrentTick();
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_BROKEN_SUN_RESOLUTION_HOLD_STARTED] tick=%llu resolution=%s anchor=%u conduit=%u holdTicks=%llu exactObjectives=true"),
            static_cast<unsigned long long>(BrokenSunResolutionStartTick),
            FEchoesBrokenSunMissionModel::ResolutionStableName(
                SelectedBrokenSunResolution),
            BrokenSunApproachAnchorId,
            BrokenSunResolutionConduitId,
            static_cast<unsigned long long>(RequiredResolutionTicks));
    }

    const bool bAnyProtectedLoss =
        !bWorkerValid || !bMaraValid || !bOruunValid || !bTalarValid ||
        !IsLocalChoir(Voice, EntityType::Soldier) ||
        !IsLocalChoir(Heavy, EntityType::HeavyUnit) ||
        !IsLocalChoir(Neme, EntityType::ScoutUnit);
    const bool bApproachBreach =
        BrokenSunApproachAnchorId != 0 && !bApproachValid;
    const bool bLockedResolutionBreach =
        SelectedBrokenSunResolution != EEchoesFinalResolution::None &&
        (!bApproachValid || !bAccordValid || !bResolutionValid);
    const bool bHoldBreach =
        bBrokenSunResolutionHoldStarted && !bConduitValid;
    if (!bLocalCoreIntact || bAnyProtectedLoss || bApproachBreach ||
        bLockedResolutionBreach || bHoldBreach ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        bBrokenSunResolutionContractFailed = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_BROKEN_SUN_CONTRACT_FAILED] tick=%llu core=%s protected=%s approach=%s accord=%s resolution=%s conduit=%s holdStarted=%s outcome=%u irreversible=true"),
            static_cast<unsigned long long>(Simulation->CurrentTick()),
            bLocalCoreIntact ? TEXT("true") : TEXT("false"),
            bAnyProtectedLoss ? TEXT("false") : TEXT("true"),
            bApproachValid ? TEXT("true") : TEXT("false"),
            bAccordValid ? TEXT("true") : TEXT("false"),
            bResolutionValid ? TEXT("true") : TEXT("false"),
            bConduitValid ? TEXT("true") : TEXT("false"),
            bBrokenSunResolutionHoldStarted ? TEXT("true") : TEXT("false"),
            static_cast<uint8>(Simulation->Outcome()));
    }
}

bool UEchoesSimulationSubsystem::IssueCommand(
    echoes::sim::CommandType CommandType,
    uint32 ActorId,
    uint32 TargetId,
    const FVector& WorldPosition,
    echoes::sim::FutureWellChoice WellChoice,
    FString& OutFeedback)
{
    return QueuePlayerCommand(
        CommandType,
        ActorId,
        TargetId,
        WorldToSim(WorldPosition),
        WellChoice,
        echoes::sim::EntityType::Barracks,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::IssueBuildCommand(
    uint32 WorkerId,
    echoes::sim::EntityType BuildingType,
    const FVector& WorldPosition,
    FString& OutFeedback)
{
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignReserveAuthority &&
        BuildingType == echoes::sim::EntityType::Dropoff)
    {
        const EEchoesReserveAuthorityPhase Phase =
            GetReserveAuthorityPhase();
        if (Phase == EEchoesReserveAuthorityPhase::SecureAuthority)
        {
            OutFeedback = TEXT("[RESERVE_AUTHORITY_REQUIRED] Mara must secure the reserve authority site before district Power Links can be placed.");
            return false;
        }
        if (Phase ==
                EEchoesReserveAuthorityPhase::ReachDeferredDistrict ||
            Phase == EEchoesReserveAuthorityPhase::Complete)
        {
            OutFeedback = TEXT("[RESERVE_ALLOCATION_LIMIT] Two districts already hold the available reserve. Reach the deferred district with Mara.");
            return false;
        }
    }
    return QueuePlayerCommand(
        echoes::sim::CommandType::Build,
        WorkerId,
        0,
        WorldToSim(WorldPosition),
        echoes::sim::FutureWellChoice::Dormant,
        BuildingType,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::IssueProductionCommand(
    uint32 ProducerId,
    echoes::sim::EntityType UnitType,
    FString& OutFeedback)
{
    const echoes::sim::Entity* Producer = FindEntity(ProducerId);
    const echoes::sim::Vec2 Position =
        Producer != nullptr ? Producer->position : echoes::sim::Vec2{};
    return QueuePlayerCommand(
        echoes::sim::CommandType::Produce,
        ProducerId,
        0,
        Position,
        echoes::sim::FutureWellChoice::Dormant,
        UnitType,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::IssueResearchCommand(
    uint32 ProducerId,
    echoes::sim::ResearchType ResearchType,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady)
    {
        OutFeedback = TEXT("[SIM_NOT_READY] The deterministic simulation is unavailable.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        ResearchType == echoes::sim::ResearchType::ChoirSharedResolution &&
        GetSeveralVoicesOneCommandPhase() !=
            EEchoesSeveralVoicesOneCommandPhase::ResearchSharedResolution)
    {
        OutFeedback = TEXT(
            "[CHOIR_VOICE_CONTRACT_REQUIRED] Resolve Possible and Manifest at their inherited sites and position Neme at command before researching Shared Resolution.");
        return false;
    }
    const echoes::sim::ResearchResult Result = Simulation->ValidateResearch(
        LocalPlayerId, ProducerId, ResearchType);
    if (Result != echoes::sim::ResearchResult::Valid)
    {
        switch (Result)
        {
            case echoes::sim::ResearchResult::InvalidPlayer:
            case echoes::sim::ResearchResult::InvalidProducer:
                OutFeedback = TEXT("[RESEARCH_PRODUCER_INVALID] Select an owned production structure.");
                break;
            case echoes::sim::ResearchResult::ProducerIncomplete:
                OutFeedback = TEXT("[RESEARCH_PRODUCER_INCOMPLETE] Construction must finish before research.");
                break;
            case echoes::sim::ResearchResult::ProducerBusy:
                OutFeedback = TEXT("[RESEARCH_BUSY] Production or another research project is active.");
                break;
            case echoes::sim::ResearchResult::InvalidTechnology:
            case echoes::sim::ResearchResult::WrongFaction:
                OutFeedback = TEXT("[RESEARCH_UNAVAILABLE] This technology is unavailable to the local faction.");
                break;
            case echoes::sim::ResearchResult::AlreadyCompleted:
                OutFeedback = TEXT("[RESEARCH_COMPLETE] This technology is already operational.");
                break;
            case echoes::sim::ResearchResult::PrerequisiteMissing:
                OutFeedback = TEXT("[RESEARCH_PREREQUISITE] Complete the preceding technology first.");
                break;
            case echoes::sim::ResearchResult::InsufficientResources:
                OutFeedback = TEXT("[INSUFFICIENT_RESOURCES] The selected research cannot be funded.");
                break;
            case echoes::sim::ResearchResult::Valid:
                break;
        }
        return false;
    }
    echoes::sim::Command Command{};
    Command.executeTick = ResolvePlayerExecuteTick(0);
    Command.player = LocalPlayerId;
    Command.sequence = NextPlayerCommandSequence++;
    Command.type = echoes::sim::CommandType::Research;
    Command.actor = ProducerId;
    Command.researchType = ResearchType;
    std::string Rejection;
    if (!Simulation->QueueCommand(Command, &Rejection))
    {
        OutFeedback = FString::Printf(
            TEXT("[RESEARCH_REJECTED] %s"), UTF8_TO_TCHAR(Rejection.c_str()));
        return false;
    }
    OutFeedback = TEXT("RESEARCH QUEUED: production is suspended until completion.");
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_RESEARCH_QUEUED] player=%u producer=%u technology=%u"),
        LocalPlayerId,
        ProducerId,
        static_cast<uint8>(ResearchType));
    return true;
}

bool UEchoesSimulationSubsystem::IssueWarformAdaptation(
    uint32 ActorId,
    uint32 SiteId,
    echoes::sim::WarformAdaptation Adaptation,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady)
    {
        OutFeedback = TEXT("[SIM_NOT_READY] The deterministic simulation is unavailable.");
        return false;
    }
    if (Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        OutFeedback = TEXT("[MATCH_FINISHED] Press R to restart before issuing orders.");
        return false;
    }
    const echoes::sim::Entity* Actor = Simulation->FindEntity(ActorId);
    if (Actor == nullptr)
    {
        OutFeedback = TEXT("[ACTOR_MISSING] The selected entity no longer exists.");
        return false;
    }
    if (Actor->owner != LocalPlayerId)
    {
        OutFeedback = TEXT("[ACTOR_NOT_OWNED] Only the local faction accepts player orders.");
        return false;
    }
    switch (Simulation->ValidateWarformAdaptation(
        LocalPlayerId, ActorId, SiteId, Adaptation))
    {
        case echoes::sim::WarformAdaptationResult::Valid:
            break;
        case echoes::sim::WarformAdaptationResult::InvalidPlayer:
        case echoes::sim::WarformAdaptationResult::InvalidActor:
            OutFeedback = TEXT("[WARFORM_REQUIRED] Select a Kharuun combat warform.");
            return false;
        case echoes::sim::WarformAdaptationResult::InvalidAdaptation:
            OutFeedback = TEXT("[ADAPTATION_INVALID] Choose Carapace or Striker form.");
            return false;
        case echoes::sim::WarformAdaptationResult::AlreadyAdapted:
            OutFeedback = TEXT("[ALREADY_ADAPTED] This warform already has the chosen form.");
            return false;
        case echoes::sim::WarformAdaptationResult::MoltActive:
            OutFeedback = TEXT("[MOLT_ACTIVE] This warform is already molting.");
            return false;
        case echoes::sim::WarformAdaptationResult::InvalidSite:
            OutFeedback = TEXT("[GROWTH_BASIN_REQUIRED] Choose a completed friendly Growth Basin.");
            return false;
        case echoes::sim::WarformAdaptationResult::OutsideSiteRadius:
            OutFeedback = TEXT("[OUTSIDE_MOLT_SITE] Move within the Growth Basin's adaptation field.");
            return false;
        case echoes::sim::WarformAdaptationResult::InsufficientDawn:
            OutFeedback = TEXT("[INSUFFICIENT_DAWN] The adaptation cannot be funded.");
            return false;
    }

    echoes::sim::Command Command;
    Command.executeTick = ResolvePlayerExecuteTick(1);
    Command.player = LocalPlayerId;
    Command.sequence = NextPlayerCommandSequence++;
    Command.type = echoes::sim::CommandType::AdaptWarform;
    Command.actor = ActorId;
    Command.target = SiteId;
    Command.position = Actor->position;
    Command.warformAdaptation = Adaptation;
    std::string Rejection;
    if (!Simulation->QueueCommand(Command, &Rejection))
    {
        OutFeedback = FString::Printf(
            TEXT("[CORE_REJECTED] %s"), UTF8_TO_TCHAR(Rejection.c_str()));
        return false;
    }
    OutFeedback = TEXT("[QUEUED] Warform molt accepted for the next simulation tick.");
    return true;
}

bool UEchoesSimulationSubsystem::IssueMineralCover(
    uint32 ActorId,
    const FVector& WorldPosition,
    FString& OutFeedback)
{
    return QueuePlayerCommand(
        echoes::sim::CommandType::RaiseMineralCover,
        ActorId,
        0,
        WorldToSim(WorldPosition),
        echoes::sim::FutureWellChoice::Dormant,
        echoes::sim::EntityType::Barracks,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::IssueChoirReconciliation(
    uint32 ActorId,
    echoes::sim::ChoirIdentityState StableState,
    FString& OutFeedback)
{
    if (StableState != echoes::sim::ChoirIdentityState::Manifest &&
        StableState != echoes::sim::ChoirIdentityState::Possible)
    {
        OutFeedback = TEXT("[CHOIR_IDENTITY_INVALID] Choose Manifest or Possible identity.");
        return false;
    }
    const echoes::sim::Entity* Actor = FindEntity(ActorId);
    return QueuePlayerCommand(
        StableState == echoes::sim::ChoirIdentityState::Manifest
            ? echoes::sim::CommandType::ReconcileToManifest
            : echoes::sim::CommandType::ReconcileToPossible,
        ActorId,
        0,
        Actor != nullptr ? Actor->position : echoes::sim::Vec2{},
        echoes::sim::FutureWellChoice::Dormant,
        echoes::sim::EntityType::Barracks,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::ChooseFinalResolution(
    EEchoesFinalResolution Resolution,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady ||
        SelectedOperation != EEchoesOperationMode::CampaignTheBrokenSun)
    {
        OutFeedback = TEXT(
            "[BROKEN_SUN_NOT_ACTIVE] Deploy The Broken Sun before committing a final resolution.");
        return false;
    }
    const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
    if (!FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            Plan,
            Resolution))
    {
        OutFeedback = FString::Printf(
            TEXT("[FINAL_RESOLUTION_UNEARNED] %s is not available under this campaign's explicit doctrine, district, and Lume protocol receipts."),
            FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                Resolution));
        return false;
    }
    if (SelectedBrokenSunResolution != EEchoesFinalResolution::None)
    {
        if (SelectedBrokenSunResolution == Resolution)
        {
            OutFeedback = FString::Printf(
                TEXT("FINAL RESOLUTION LOCKED FOR THIS OPERATION: %s. Raise the Resolution Conduit at the marked convergence and hold the contract."),
                FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                    Resolution));
            return true;
        }
        OutFeedback = FString::Printf(
            TEXT("[FINAL_RESOLUTION_IRREVERSIBLE] %s is already committed for this operation; %s cannot replace it."),
            FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                SelectedBrokenSunResolution),
            FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                Resolution));
        return false;
    }
    if (GetBrokenSunPhase() !=
        EEchoesBrokenSunPhase::ChooseFinalResolution)
    {
        OutFeedback = TEXT(
            "[FINAL_RESOLUTION_PREREQUISITES] Secure the Crownfall approach and assemble the three-site accord before choosing the campaign resolution.");
        return false;
    }
    if (PendingBrokenSunResolution != Resolution)
    {
        PendingBrokenSunResolution = Resolution;
        OutFeedback = FString::Printf(
            TEXT("ARMED %s — %s Press the same resolution command again to make this operation's irreversible commitment."),
            FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                Resolution),
            FEchoesBrokenSunMissionModel::ResolutionCostSummary(
                Resolution));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_BROKEN_SUN_RESOLUTION_ARMED] resolution=%s planKey=%u availability=0x%02X confirmationRequired=true"),
            FEchoesBrokenSunMissionModel::ResolutionStableName(
                Resolution),
            Plan.StablePlanKey,
            Plan.AvailableFinalResolutions);
        return true;
    }

    SelectedBrokenSunResolution = Resolution;
    OutFeedback = FString::Printf(
        TEXT("FINAL RESOLUTION LOCKED FOR THIS OPERATION: %s — %s Raise the Resolution Conduit at the marked convergence. The campaign ending is not recorded until the operation succeeds."),
        FEchoesBrokenSunMissionModel::ResolutionDisplayName(Resolution),
        FEchoesBrokenSunMissionModel::ResolutionCostSummary(Resolution));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_BROKEN_SUN_RESOLUTION_LOCKED] resolution=%s planKey=%u availability=0x%02X operationIntentIrreversible=true campaignLedgerCommitted=false"),
        FEchoesBrokenSunMissionModel::ResolutionStableName(Resolution),
        Plan.StablePlanKey,
        Plan.AvailableFinalResolutions);
    return true;
}

bool UEchoesSimulationSubsystem::QueuePlayerCommand(
    echoes::sim::CommandType CommandType,
    uint32 ActorId,
    uint32 TargetId,
    const echoes::sim::Vec2& SimPosition,
    echoes::sim::FutureWellChoice WellChoice,
    echoes::sim::EntityType BuildType,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady)
    {
        OutFeedback = TEXT("[SIM_NOT_READY] The deterministic simulation is unavailable.");
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_CMD_SIM_NOT_READY] actor=%u"), ActorId);
        return false;
    }
    if (Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        OutFeedback = TEXT("[MATCH_FINISHED] Press R to restart before issuing orders.");
        return false;
    }

    const echoes::sim::Entity* Actor = Simulation->FindEntity(ActorId);
    if (Actor == nullptr)
    {
        OutFeedback = TEXT("[ACTOR_MISSING] The selected entity no longer exists.");
        return false;
    }
    if (Actor->owner != LocalPlayerId)
    {
        OutFeedback = TEXT("[ACTOR_NOT_OWNED] Only locally owned units accept player orders.");
        return false;
    }

    if (!ValidatePrototypeCommand(
            CommandType,
            *Actor,
            TargetId,
            SimPosition,
            WellChoice,
            BuildType,
            OutFeedback))
    {
        UE_LOG(
            LogEchoes,
            Verbose,
            TEXT("[ECHOES_CMD_VALIDATION_REJECTED] actor=%u type=%u detail=%s"),
            ActorId,
            static_cast<uint8>(CommandType),
            *OutFeedback);
        return false;
    }

    echoes::sim::Command Command;
    Command.executeTick = ResolvePlayerExecuteTick(1);
    Command.player = LocalPlayerId;
    Command.sequence = NextPlayerCommandSequence++;
    Command.type = CommandType;
    Command.actor = ActorId;
    Command.target = TargetId;
    Command.position = SimPosition;
    Command.wellChoice = WellChoice;
    Command.buildType = BuildType;

    std::string Rejection;
    if (!Simulation->QueueCommand(Command, &Rejection))
    {
        OutFeedback = FString::Printf(
            TEXT("[CORE_REJECTED] %s"),
            UTF8_TO_TCHAR(Rejection.c_str()));
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_CMD_CORE_REJECTED] actor=%u sequence=%llu reason=%s"),
            ActorId,
            static_cast<unsigned long long>(Command.sequence),
            UTF8_TO_TCHAR(Rejection.c_str()));
        return false;
    }

    OutFeedback = TEXT("[QUEUED] Order accepted for the next simulation tick.");
    UE_LOG(
        LogEchoes,
        Verbose,
        TEXT("[ECHOES_CMD_QUEUED] tick=%llu actor=%u type=%u target=%u sequence=%llu"),
        static_cast<unsigned long long>(Command.executeTick),
        Command.actor,
        static_cast<uint8>(Command.type),
        Command.target,
        static_cast<unsigned long long>(Command.sequence));
    return true;
}

echoes::sim::Tick UEchoesSimulationSubsystem::ResolvePlayerExecuteTick(
    echoes::sim::Tick OfflineDelayTicks) const
{
    if (!Simulation.IsValid())
    {
        return 0;
    }
    const echoes::sim::Tick DelayTicks =
        bNetworkHumanOpponent ? 3 : OfflineDelayTicks;
    const echoes::sim::Tick CurrentTick = Simulation->CurrentTick();
    if (DelayTicks >
        std::numeric_limits<echoes::sim::Tick>::max() - CurrentTick)
    {
        return std::numeric_limits<echoes::sim::Tick>::max();
    }
    return CurrentTick + DelayTicks;
}

bool UEchoesSimulationSubsystem::ValidatePrototypeCommand(
    echoes::sim::CommandType CommandType,
    const echoes::sim::Entity& Actor,
    uint32 TargetId,
    const echoes::sim::Vec2& Position,
    echoes::sim::FutureWellChoice WellChoice,
    echoes::sim::EntityType BuildType,
    FString& OutFeedback) const
{
    using echoes::sim::CommandType;
    using echoes::sim::EntityType;
    using echoes::sim::FutureWellChoice;

    const echoes::sim::Entity* Target =
        TargetId != 0 ? Simulation->FindEntity(TargetId) : nullptr;
    switch (CommandType)
    {
        case CommandType::Stop:
            return true;
        case CommandType::Move:
            if (Actor.waystoneMode != echoes::sim::WaystoneMode::NotWaystone &&
                Actor.waystoneMode != echoes::sim::WaystoneMode::Mobile)
            {
                OutFeedback = TEXT("[WAYSTONE_MUST_BE_MOBILE] Uproot the Waystone before moving it.");
                return false;
            }
            if (Actor.movementPerTickRaw <= 0)
            {
                OutFeedback = TEXT("[IMMOBILE_ACTOR] This entity cannot move.");
                return false;
            }
            if (!Simulation->IsPositionPassable(Position))
            {
                OutFeedback = TEXT("[INVALID_DESTINATION] The destination is outside or blocked.");
                return false;
            }
            return true;
        case CommandType::AttackMove:
            if (Actor.movementPerTickRaw <= 0 || Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[ATTACK_MOVE_REQUIRES_COMBAT_UNIT] Select a mobile combat unit.");
                return false;
            }
            if (!Simulation->IsPositionPassable(Position))
            {
                OutFeedback = TEXT("[INVALID_DESTINATION] The attack-move destination is outside or blocked.");
                return false;
            }
            return true;
        case CommandType::Patrol:
            if (Actor.movementPerTickRaw <= 0 || Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[PATROL_REQUIRES_COMBAT_UNIT] Select a mobile combat unit.");
                return false;
            }
            if (!Simulation->IsPositionPassable(Position))
            {
                OutFeedback = TEXT("[INVALID_DESTINATION] The patrol endpoint is outside or blocked.");
                return false;
            }
            if (Position == Actor.position)
            {
                OutFeedback = TEXT("[PATROL_ENDPOINT_UNCHANGED] Choose a different patrol endpoint.");
                return false;
            }
            return true;
        case CommandType::ToggleDeploy:
            if (Actor.faction != echoes::sim::Faction::MeridianCompact ||
                Actor.type != EntityType::HeavyUnit)
            {
                OutFeedback = TEXT("[BULWARK_REQUIRED] Deployment requires a Meridian Bulwark Team.");
                return false;
            }
            if (!Actor.deployed && Position == Actor.position)
            {
                OutFeedback = TEXT("[DEPLOY_FACING_REQUIRED] Point away from the Bulwark to set cover facing.");
                return false;
            }
            return true;
        case CommandType::ActivateRelaySupply:
            switch (Simulation->ValidateRelaySupply(LocalPlayerId, Actor.id))
            {
                case echoes::sim::RelaySupplyResult::Valid:
                    return true;
                case echoes::sim::RelaySupplyResult::InvalidPlayer:
                case echoes::sim::RelaySupplyResult::InvalidActor:
                    OutFeedback = TEXT("[RELAY_REQUIRED] Select a Meridian Relay Skiff.");
                    break;
                case echoes::sim::RelaySupplyResult::AlreadyActive:
                    OutFeedback = TEXT("[RELAY_ALREADY_ACTIVE] This Relay is already extending logistics.");
                    break;
                case echoes::sim::RelaySupplyResult::CooldownActive:
                    OutFeedback = TEXT("[RELAY_COOLDOWN] This Relay has not recovered its reserve.");
                    break;
                case echoes::sim::RelaySupplyResult::Disconnected:
                    OutFeedback = TEXT("[RELAY_DISCONNECTED] Move within range of an Anchor or Power Link.");
                    break;
            }
            return false;
        case CommandType::ToggleWaystoneRoot:
            switch (Simulation->ValidateWaystoneRoot(LocalPlayerId, Actor.id))
            {
                case echoes::sim::WaystoneRootResult::Valid:
                    return true;
                case echoes::sim::WaystoneRootResult::InvalidPlayer:
                case echoes::sim::WaystoneRootResult::InvalidActor:
                    OutFeedback = TEXT("[WAYSTONE_REQUIRED] Select a Kharuun Waystone.");
                    break;
                case echoes::sim::WaystoneRootResult::TransitionActive:
                    OutFeedback = TEXT("[WAYSTONE_TRANSITION_ACTIVE] The Waystone is already changing state.");
                    break;
                case echoes::sim::WaystoneRootResult::RootingBlocked:
                    OutFeedback = TEXT("[WAYSTONE_ROOTING_BLOCKED] Move to a clear, passable footprint.");
                    break;
            }
            return false;
        case CommandType::AdaptWarform:
            OutFeedback = TEXT("[ADAPTATION_FORM_REQUIRED] Use a declared warform adaptation command.");
            return false;
        case CommandType::RaiseMineralCover:
            switch (Simulation->ValidateMineralCover(
                LocalPlayerId, Actor.id, Position))
            {
                case echoes::sim::MineralCoverResult::Valid:
                    return true;
                case echoes::sim::MineralCoverResult::InvalidPlayer:
                case echoes::sim::MineralCoverResult::InvalidActor:
                    OutFeedback = TEXT("[CAIRNBACK_REQUIRED] Select a Kharuun Cairnback.");
                    break;
                case echoes::sim::MineralCoverResult::MoltActive:
                    OutFeedback = TEXT("[MOLT_ACTIVE] A molting Cairnback cannot raise cover.");
                    break;
                case echoes::sim::MineralCoverResult::CooldownActive:
                    OutFeedback = TEXT("[MINERAL_COVER_COOLDOWN] This Cairnback has not regrown its mineral reserve.");
                    break;
                case echoes::sim::MineralCoverResult::OutsideCastRange:
                    OutFeedback = TEXT("[MINERAL_COVER_OUT_OF_RANGE] Choose a position closer to the Cairnback.");
                    break;
                case echoes::sim::MineralCoverResult::InvalidPosition:
                    OutFeedback = TEXT("[MINERAL_COVER_TERRAIN_INVALID] Choose an open or scarred battlefield position.");
                    break;
                case echoes::sim::MineralCoverResult::Occupied:
                    OutFeedback = TEXT("[MINERAL_COVER_OCCUPIED] The mineral barrier needs a clear footprint.");
                    break;
                case echoes::sim::MineralCoverResult::InsufficientDawn:
                    OutFeedback = TEXT("[INSUFFICIENT_DAWN] The mineral barrier cannot be funded.");
                    break;
                case echoes::sim::MineralCoverResult::EntityCapacityReached:
                    OutFeedback = TEXT("[ENTITY_CAPACITY_REACHED] No additional battlefield object can be created.");
                    break;
            }
            return false;
        case CommandType::ReconcileToManifest:
        case CommandType::ReconcileToPossible:
        {
            if (SelectedOperation != EEchoesOperationMode::Skirmish &&
                SelectedOperation !=
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
                SelectedOperation !=
                    EEchoesOperationMode::CampaignTheBrokenSun)
            {
                OutFeedback = TEXT("[CHOIR_COMMAND_AUTHORITY_REQUIRED] Reconciliation requires a player-commanded Hollow Choir force.");
                return false;
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand ||
                SelectedOperation ==
                    EEchoesOperationMode::CampaignTheBrokenSun)
            {
                const echoes::sim::PlayerState* Player =
                    Simulation->FindPlayer(LocalPlayerId);
                if (Player == nullptr ||
                    !Player->HasCompletedResearch(
                        echoes::sim::ResearchType::ChoirHeldAlternatives))
                {
                    OutFeedback = TEXT("[CHOIR_HELD_ALTERNATIVES_REQUIRED] Research Held Alternatives before resolving a protected voice.");
                    return false;
                }
            }
            const echoes::sim::ChoirIdentityState StableState =
                CommandType == CommandType::ReconcileToManifest
                    ? echoes::sim::ChoirIdentityState::Manifest
                    : echoes::sim::ChoirIdentityState::Possible;
            switch (Simulation->ValidateChoirReconciliation(
                LocalPlayerId, Actor.id, StableState))
            {
                case echoes::sim::ChoirReconciliationResult::Valid:
                    return true;
                case echoes::sim::ChoirReconciliationResult::InvalidPlayer:
                case echoes::sim::ChoirReconciliationResult::InvalidActor:
                    OutFeedback = TEXT("[CHOIR_IDENTITY_REQUIRED] Select a completed Hollow Choir identity unit.");
                    break;
                case echoes::sim::ChoirReconciliationResult::AlreadyResolving:
                    OutFeedback = TEXT("[CHOIR_RECONCILING] This voice is already resolving between identities.");
                    break;
                case echoes::sim::ChoirReconciliationResult::AlreadyStable:
                    OutFeedback = TEXT("[CHOIR_ALREADY_STABLE] This voice already holds the requested identity.");
                    break;
                case echoes::sim::ChoirReconciliationResult::CooldownActive:
                    OutFeedback = TEXT("[CHOIR_RECONCILIATION_COOLDOWN] This voice cannot reconcile again yet.");
                    break;
                case echoes::sim::ChoirReconciliationResult::InsufficientDawn:
                    OutFeedback = TEXT("[INSUFFICIENT_DAWN] The Choir cannot fund this reconciliation.");
                    break;
            }
            return false;
        }
        case CommandType::Research:
            OutFeedback = TEXT("[RESEARCH_FORM_REQUIRED] Use a declared research command.");
            return false;
        case CommandType::Hold:
            if (Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[HOLD_REQUIRES_DEFENDER] Select an attack-capable unit.");
                return false;
            }
            return true;
        case CommandType::Guard:
            if (Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[GUARD_REQUIRES_DEFENDER] Select an attack-capable unit.");
                return false;
            }
            if (Target == nullptr || Target->owner != LocalPlayerId ||
                Target->id == Actor.id)
            {
                OutFeedback = TEXT("[GUARD_TARGET_INVALID] Guard requires a different live owned entity.");
                return false;
            }
            return true;
        case CommandType::Gather:
            if (Actor.type != EntityType::Worker || Target == nullptr ||
                Target->type != EntityType::ResourceNode ||
                Target->resourceRemaining <= 0)
            {
                OutFeedback = TEXT("[GATHER_INVALID] Select a worker and an available Matter node.");
                return false;
            }
            break;
        case CommandType::Deliver:
            if (Actor.type != EntityType::Worker || Actor.cargo <= 0 ||
                Target == nullptr || Target->owner != LocalPlayerId ||
                (Target->type != EntityType::CommandCore &&
                 Target->type != EntityType::Dropoff) ||
                !Target->completed)
            {
                OutFeedback = TEXT("[DELIVER_INVALID] A worker carrying Matter needs a completed drop-off.");
                return false;
            }
            return true;
        case CommandType::Attack:
            if (Actor.attackDamage <= 0 || Target == nullptr ||
                Target->owner == echoes::sim::kNeutralPlayer ||
                Target->owner == LocalPlayerId)
            {
                OutFeedback = TEXT("[ATTACK_INVALID] The actor or target cannot be used for this attack.");
                return false;
            }
            break;
        case CommandType::FutureWell:
            if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
                GetProloguePhase() != EEchoesProloguePhase::DecideFutureWell)
            {
                OutFeedback = TEXT("[ARCHIVE_REQUIRED] Mara Vey's archive carrier must hold the recovery site at tile 22,18 before a Well protocol can be committed.");
                return false;
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignChoirAtLumeReach &&
                (GetChoirAtLumeReachPhase() !=
                     EEchoesChoirAtLumeReachPhase::CommitFutureWell ||
                 TargetId != ChoirAtLumeReachWellId))
            {
                OutFeedback = TEXT("[LUME_REACH_ANCHORS_REQUIRED] Resolve the inherited liability and raise both public Listening Spines before committing this operation's Future Well.");
                return false;
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignNoNeutralLedger)
            {
                const FEchoesNoNeutralLedgerPlan Plan =
                    GetNoNeutralLedgerPlan();
                if (GetNoNeutralLedgerPhase() !=
                        EEchoesNoNeutralLedgerPhase::ApplyRecordedProtocol ||
                    TargetId != NoNeutralWellId ||
                    WellChoice != Plan.LumeProtocol)
                {
                    OutFeedback = FString::Printf(
                        TEXT("[NO_NEUTRAL_LEDGER_PROTOCOL_REQUIRED] Attest both evidence channels, then apply only the recorded %s protocol."),
                        Plan.ProtocolDisplayName);
                    return false;
                }
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignFutureThatWon)
            {
                const FEchoesFutureThatWonPlan Plan =
                    GetFutureThatWonPlan();
                if (GetFutureThatWonPhase() !=
                        EEchoesFutureThatWonPhase::BindRecordedProtocol ||
                    TargetId != FutureWonWellId ||
                    WellChoice != Plan.RecordedProtocol)
                {
                    OutFeedback = FString::Printf(
                        TEXT("[FUTURE_THAT_WON_PROTOCOL_REQUIRED] Establish independent readback and verify both recorded district inputs, then bind only the recorded %s protocol."),
                        Plan.ProtocolDisplayName);
                    return false;
                }
            }
            if (Actor.type != EntityType::Worker || Target == nullptr ||
                Target->type != EntityType::FutureWell ||
                Target->wellChoice != FutureWellChoice::Dormant ||
                WellChoice == FutureWellChoice::Dormant)
            {
                OutFeedback = TEXT("[WELL_INVALID] A worker must target a dormant Future Well with a chosen protocol.");
                return false;
            }
            break;
        case CommandType::Build:
        {
            if (Actor.type != EntityType::Worker)
            {
                OutFeedback = TEXT("[BUILD_REQUIRES_WORKER] Select a worker before placing a structure.");
                return false;
            }
            if (Actor.order.type == echoes::sim::OrderType::Build)
            {
                OutFeedback = TEXT("[WORKER_BUSY] This worker already has a construction order.");
                return false;
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignNamesWithoutBirths &&
                BuildType == EntityType::Dropoff &&
                GetNamesWithoutBirthsPhase() ==
                    EEchoesNamesWithoutBirthsPhase::LocateCensus)
            {
                OutFeedback = TEXT("[CENSUS_TRACE_REQUIRED] Talar must reach the inherited census site before a Power Link can stabilize its archive.");
                return false;
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignChoirAtLumeReach &&
                BuildType == EntityType::UtilityStructure)
            {
                const EEchoesChoirAtLumeReachPhase Phase =
                    GetChoirAtLumeReachPhase();
                const FEchoesChoirAtLumeReachPlan Plan =
                    GetChoirAtLumeReachPlan();
                const Vec2 RequiredSite =
                    Phase == EEchoesChoirAtLumeReachPhase::RaiseFirstAnchor
                        ? Plan.FirstAnchorSite
                    : Phase ==
                          EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor
                        ? Plan.SecondAnchorSite
                        : Vec2{};
                if (Phase !=
                        EEchoesChoirAtLumeReachPhase::RaiseFirstAnchor &&
                    Phase !=
                        EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor)
                {
                    OutFeedback = TEXT("[LUME_REACH_ANCHOR_SEQUENCE] Establish contact and root the Waystone at the deferred liability before raising the two Listening Spines in sequence.");
                    return false;
                }
                if (!IsWithinTiles(
                        Position,
                        RequiredSite,
                        ChoirAtLumeReachSiteRadiusTiles))
                {
                    OutFeedback = FString::Printf(
                        TEXT("[LUME_REACH_ANCHOR_SITE] Place the active Listening Spine within %d tiles of the public anchor at %d,%d."),
                        ChoirAtLumeReachSiteRadiusTiles,
                        RequiredSite.x.FloorToInt(),
                        RequiredSite.y.FloorToInt());
                    return false;
                }
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignNoNeutralLedger &&
                BuildType == EntityType::UtilityStructure)
            {
                const FEchoesNoNeutralLedgerPlan Plan =
                    GetNoNeutralLedgerPlan();
                const FEchoesObjectiveSnapshot Objective =
                    GetLocalObjectiveSnapshot();
                if (GetNoNeutralLedgerPhase() !=
                    EEchoesNoNeutralLedgerPhase::IntegrateDistrictContributions)
                {
                    OutFeedback = TEXT("[NO_NEUTRAL_LEDGER_ROUTE_REQUIRED] Root the Waystone at the inherited route before linking the two recorded district contributions.");
                    return false;
                }
                const bool bFirstOutstanding =
                    !Objective.bNoNeutralFirstDistrictIntegrated &&
                    IsWithinTiles(
                        Position,
                        Plan.FirstDistrictSite,
                        NoNeutralLedgerSiteRadiusTiles);
                const bool bSecondOutstanding =
                    !Objective.bNoNeutralSecondDistrictIntegrated &&
                    IsWithinTiles(
                        Position,
                        Plan.SecondDistrictSite,
                        NoNeutralLedgerSiteRadiusTiles);
                if (!bFirstOutstanding && !bSecondOutstanding)
                {
                    OutFeedback = FString::Printf(
                        TEXT("[NO_NEUTRAL_LEDGER_DISTRICT_SITE] Place a Listening Spine within %d tiles of either outstanding recorded district interface."),
                        NoNeutralLedgerSiteRadiusTiles);
                    return false;
                }
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignFutureThatWon &&
                BuildType == EntityType::UtilityStructure)
            {
                const FEchoesFutureThatWonPlan Plan =
                    GetFutureThatWonPlan();
                const FEchoesObjectiveSnapshot Objective =
                    GetLocalObjectiveSnapshot();
                if (GetFutureThatWonPhase() !=
                    EEchoesFutureThatWonPhase::VerifyRecordedInputs)
                {
                    OutFeedback = TEXT("[FUTURE_THAT_WON_READBACK_REQUIRED] Position Oruun and the independent verifier at the two public evidence readbacks before linking the recorded district inputs.");
                    return false;
                }
                const bool bFirstOutstanding =
                    !Objective.bFutureWonFirstInputVerified &&
                    IsWithinTiles(
                        Position,
                        Plan.FirstDistrictInputSite,
                        FutureThatWonSiteRadiusTiles);
                const bool bSecondOutstanding =
                    !Objective.bFutureWonSecondInputVerified &&
                    IsWithinTiles(
                        Position,
                        Plan.SecondDistrictInputSite,
                        FutureThatWonSiteRadiusTiles);
                if (!bFirstOutstanding && !bSecondOutstanding)
                {
                    OutFeedback = FString::Printf(
                        TEXT("[FUTURE_THAT_WON_DISTRICT_SITE] Place a Kharuun Listening Spine within %d tiles of either outstanding recorded district input."),
                        FutureThatWonSiteRadiusTiles);
                    return false;
                }
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
                BuildType == EntityType::UtilityStructure)
            {
                const FEchoesAssemblyOfTheMissingPlan Plan =
                    GetAssemblyOfTheMissingPlan();
                if (GetAssemblyOfTheMissingPhase() !=
                    EEchoesAssemblyOfTheMissingPhase::LinkCrownfallIndex)
                {
                    OutFeedback = TEXT("[ASSEMBLY_PUBLIC_READBACK_REQUIRED] Position Oruun and the independent verifier at the separate public record interfaces before linking the Crownfall index.");
                    return false;
                }
                if (!IsWithinTiles(
                        Position,
                        Plan.CrownfallIndexSite,
                        AssemblyOfTheMissingSiteRadiusTiles))
                {
                    OutFeedback = FString::Printf(
                        TEXT("[ASSEMBLY_CROWNFALL_INDEX_SITE] Place a Kharuun Listening Spine within %d tiles of the neutral Crownfall public index."),
                        AssemblyOfTheMissingSiteRadiusTiles);
                    return false;
                }
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
                BuildType == EntityType::UtilityStructure)
            {
                const FEchoesSeveralVoicesOneCommandPlan Plan =
                    GetSeveralVoicesOneCommandPlan();
                if (GetSeveralVoicesOneCommandPhase() !=
                    EEchoesSeveralVoicesOneCommandPhase::AnchorCrisis)
                {
                    OutFeedback = TEXT("[CHOIR_CRISIS_CONTRACT_REQUIRED] Complete both Choir researches, resolve the protected voices into Possible and Manifest, and position Neme before raising the Phase Anchor.");
                    return false;
                }
                if (!IsWithinTiles(
                        Position,
                        Plan.CrisisAnchorSite,
                        SeveralVoicesOneCommandSiteRadiusTiles))
                {
                    OutFeedback = FString::Printf(
                        TEXT("[CHOIR_PHASE_ANCHOR_SITE] Place the Phase Anchor within %d tiles of the inherited crisis site."),
                        SeveralVoicesOneCommandSiteRadiusTiles);
                    return false;
                }
            }
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignTheBrokenSun &&
                BuildType == EntityType::UtilityStructure)
            {
                const FEchoesBrokenSunPlan Plan = GetBrokenSunPlan();
                const EEchoesBrokenSunPhase Phase = GetBrokenSunPhase();
                const echoes::sim::Vec2 ResolutionSite =
                    FEchoesBrokenSunMissionModel::
                        ResolutionConvergenceSite(
                            Plan,
                            SelectedBrokenSunResolution);
                const bool bApproachPlacement =
                    Phase ==
                        EEchoesBrokenSunPhase::SecureCrownfallApproach &&
                    IsWithinTiles(
                        Position,
                        Plan.CrownfallApproachSite,
                        BrokenSunSiteRadiusTiles);
                const bool bConduitPlacement =
                    Phase ==
                        EEchoesBrokenSunPhase::RaiseResolutionConduit &&
                    SelectedBrokenSunResolution !=
                        EEchoesFinalResolution::None &&
                    IsWithinTiles(
                        Position,
                        ResolutionSite,
                        BrokenSunConvergenceRadiusTiles);
                if (!bApproachPlacement && !bConduitPlacement)
                {
                    const echoes::sim::Vec2 RequiredSite =
                        Phase ==
                            EEchoesBrokenSunPhase::RaiseResolutionConduit
                            ? ResolutionSite
                            : Plan.CrownfallApproachSite;
                    const int32 RequiredRadius =
                        Phase ==
                                EEchoesBrokenSunPhase::RaiseResolutionConduit
                            ? BrokenSunConvergenceRadiusTiles
                            : BrokenSunSiteRadiusTiles;
                    OutFeedback = FString::Printf(
                        TEXT("[BROKEN_SUN_OBJECTIVE_SITE] The ordered contract accepts a Listening Spine only for the current objective within %d tiles of %d,%d."),
                        RequiredRadius,
                        RequiredSite.x.FloorToInt(),
                        RequiredSite.y.FloorToInt());
                    return false;
                }
            }
            const echoes::sim::PlacementResult Placement =
                Simulation->ValidatePlacement(LocalPlayerId, BuildType, Position);
            if (Placement != echoes::sim::PlacementResult::Valid)
            {
                OutFeedback = FString::Printf(
                    TEXT("[BUILD_PLACEMENT_INVALID] Placement rejected with code %u."),
                    static_cast<uint8>(Placement));
                return false;
            }
            const echoes::sim::PlayerState* Player =
                Simulation->FindPlayer(LocalPlayerId);
            const echoes::sim::ResourcePool Cost =
                Simulation->BuildCost(Actor.faction, BuildType);
            if (Player == nullptr || Player->resources.material < Cost.material ||
                Player->resources.dawnshards < Cost.dawnshards)
            {
                OutFeedback = FString::Printf(
                    TEXT("[INSUFFICIENT_RESOURCES] Requires %d Matter and %d Dawnshards."),
                    Cost.material,
                    Cost.dawnshards);
                return false;
            }
            return true;
        }
        case CommandType::Produce:
        {
            const echoes::sim::ProductionResult Result =
                Simulation->ValidateProduction(
                    LocalPlayerId,
                    Actor.id,
                    BuildType);
            switch (Result)
            {
                case echoes::sim::ProductionResult::Valid:
                    return true;
                case echoes::sim::ProductionResult::InvalidPlayer:
                case echoes::sim::ProductionResult::InvalidProducer:
                    OutFeedback = TEXT("[PRODUCER_INVALID] Select an owned production structure.");
                    break;
                case echoes::sim::ProductionResult::ProducerIncomplete:
                    OutFeedback = TEXT("[PRODUCER_INCOMPLETE] Construction must finish before production.");
                    break;
                case echoes::sim::ProductionResult::ProducerBusy:
                    OutFeedback = TEXT("[PRODUCER_BUSY] This structure already has an active production order.");
                    break;
                case echoes::sim::ProductionResult::UnsupportedUnit:
                    OutFeedback = TEXT("[UNIT_UNSUPPORTED] Command Cores produce workers; Barracks produce soldiers.");
                    break;
                case echoes::sim::ProductionResult::InsufficientResources:
                    OutFeedback = TEXT("[INSUFFICIENT_RESOURCES] The selected unit cannot be funded.");
                    break;
                case echoes::sim::ProductionResult::CapacityReached:
                    OutFeedback = TEXT("[LOGISTICS_CAPACITY] Build a drop-off before adding more units.");
                    break;
                case echoes::sim::ProductionResult::EntityCapacityReached:
                    OutFeedback = TEXT("[ENTITY_CAPACITY] The deterministic entity limit was reached.");
                    break;
            }
            return false;
        }
    }

    if (Target == nullptr || !Simulation->IsEntityVisibleTo(LocalPlayerId, Target->id))
    {
        OutFeedback = TEXT("[TARGET_NOT_VISIBLE] The simulation does not currently expose that target to the player.");
        return false;
    }
    return true;
}

bool UEchoesSimulationSubsystem::SyncEntityViews(bool bTeleportNewViews)
{
    if (!Simulation.IsValid() || GetWorld() == nullptr)
    {
        return false;
    }

    bool bAllVisibleViewsReady = true;
    TSet<uint32> LiveEntityIds;
    LiveEntityIds.Reserve(static_cast<int32>(Simulation->Entities().size()));
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (!Simulation->IsEntityVisibleTo(LocalPlayerId, Entity.id))
        {
            continue;
        }
        LiveEntityIds.Add(Entity.id);
        AEchoesEntityView* View = FindEntityView(Entity.id);
        bool bNewView = false;
        if (View == nullptr)
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            View = GetWorld()->SpawnActor<AEchoesEntityView>(
                AEchoesEntityView::StaticClass(),
                SimToWorld(Entity.position),
                FRotator::ZeroRotator,
                SpawnParameters);
            if (View == nullptr)
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_VIEW_SPAWN_FAILED] entity=%u"),
                    Entity.id);
                bAllVisibleViewsReady = false;
                continue;
            }
            EntityViews.Add(Entity.id, View);
            bNewView = true;
        }
        View->ApplyAuthoritativeState(Entity, bTeleportNewViews || bNewView);
    }

    TArray<uint32> RemovedIds;
    for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair : EntityViews)
    {
        if (!LiveEntityIds.Contains(Pair.Key))
        {
            if (AEchoesEntityView* View = Pair.Value.Get())
            {
                const bool bAuthoritativelyRemoved =
                    !bTeleportNewViews &&
                    Simulation->FindEntity(Pair.Key) == nullptr &&
                    View->GetEntityType() != echoes::sim::EntityType::ResourceNode &&
                    View->GetEntityType() != echoes::sim::EntityType::FutureWell &&
                    !View->IsTemporaryMineralCover();
                if (bAuthoritativelyRemoved)
                {
                    FActorSpawnParameters SpawnParameters;
                    SpawnParameters.ObjectFlags |= RF_Transient;
                    SpawnParameters.SpawnCollisionHandlingOverride =
                        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    if (AEchoesDestructionView* Destruction =
                            GetWorld()->SpawnActor<AEchoesDestructionView>(
                                View->GetActorLocation(),
                                FRotator::ZeroRotator,
                                SpawnParameters))
                    {
                        const UEchoesGameUserSettings* Settings =
                            UEchoesGameUserSettings::Get();
                        const bool bReducedMotion =
                            Settings != nullptr && Settings->IsReducedMotionEnabled();
                        const bool bReducedFlashing =
                            Settings != nullptr && Settings->IsReducedFlashingEnabled();
                        Destruction->InitializeDestruction(
                            View->GetEntityFaction(),
                            View->GetEntityType(),
                            bReducedMotion,
                            bReducedFlashing);
                        if (UEchoesPresentationAudioSubsystem* Audio =
                                GetWorld()->GetSubsystem<
                                    UEchoesPresentationAudioSubsystem>())
                        {
                            Audio->PlayDestruction(
                                View->GetEntityFaction(),
                                View->GetActorLocation());
                        }
                        UE_LOG(
                            LogEchoes,
                            Display,
                            TEXT("[ECHOES_DESTRUCTION_VFX] revision=destruction-vfx-v1 entity=%u authored=%s reducedMotion=%s reducedFlashing=%s collision=false navigation=false authoritative=false finalArt=false"),
                            Pair.Key,
                            Destruction->IsUsingAuthoredVFXAssets()
                                ? TEXT("true")
                                : TEXT("false"),
                            bReducedMotion ? TEXT("true") : TEXT("false"),
                            bReducedFlashing ? TEXT("true") : TEXT("false"));
                    }
                }
                View->Destroy();
            }
            RemovedIds.Add(Pair.Key);
        }
    }
    for (const uint32 RemovedId : RemovedIds)
    {
        EntityViews.Remove(RemovedId);
    }
    return bAllVisibleViewsReady;
}

bool UEchoesSimulationSubsystem::SpawnFogView()
{
    if (!Simulation.IsValid() || GetWorld() == nullptr)
    {
        return false;
    }
    DestroyFogView();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesFogView* NewFogView = GetWorld()->SpawnActor<AEchoesFogView>(
        AEchoesFogView::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (NewFogView == nullptr ||
        !NewFogView->InitializeFog(
            *Simulation,
            LocalPlayerId,
            TileWorldSize))
    {
        if (NewFogView != nullptr)
        {
            NewFogView->Destroy();
        }
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_FOG_INIT_FAILED]"));
        return false;
    }
    FogView = NewFogView;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_FOG_READY] tiles=%d visible=%d explored=%d unexplored=%d"),
        NewFogView->GetVisibleTileCount() +
            NewFogView->GetExploredTileCount() +
            NewFogView->GetUnexploredTileCount(),
        NewFogView->GetVisibleTileCount(),
        NewFogView->GetExploredTileCount(),
        NewFogView->GetUnexploredTileCount());
    return true;
}

bool UEchoesSimulationSubsystem::SpawnTerrainView()
{
    if (!Simulation.IsValid() || GetWorld() == nullptr)
    {
        return false;
    }
    DestroyTerrainView();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesTerrainView* NewTerrainView = GetWorld()->SpawnActor<AEchoesTerrainView>(
        AEchoesTerrainView::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (NewTerrainView == nullptr ||
        !NewTerrainView->InitializeTerrain(*Simulation, TileWorldSize))
    {
        if (NewTerrainView != nullptr)
        {
            NewTerrainView->Destroy();
        }
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_TERRAIN_VIEW_INIT_FAILED]"));
        return false;
    }
    TerrainView = NewTerrainView;
    return true;
}

bool UEchoesSimulationSubsystem::SyncTerrainView()
{
    AEchoesTerrainView* View = TerrainView.Get();
    return Simulation.IsValid() && View != nullptr &&
           View->SyncTerrain(*Simulation);
}

void UEchoesSimulationSubsystem::DestroyTerrainView()
{
    if (AEchoesTerrainView* View = TerrainView.Get())
    {
        View->Destroy();
    }
    TerrainView.Reset();
}

bool UEchoesSimulationSubsystem::SyncFogView()
{
    AEchoesFogView* View = FogView.Get();
    return Simulation.IsValid() && View != nullptr &&
           View->SyncVisibility(*Simulation);
}

void UEchoesSimulationSubsystem::DestroyFogView()
{
    if (AEchoesFogView* View = FogView.Get())
    {
        View->Destroy();
    }
    FogView.Reset();
}

void UEchoesSimulationSubsystem::DestroyEntityViews()
{
    for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair : EntityViews)
    {
        if (AEchoesEntityView* View = Pair.Value.Get())
        {
            View->Destroy();
        }
    }
    EntityViews.Reset();
}

const echoes::sim::Simulation* UEchoesSimulationSubsystem::GetSimulation() const
{
    return Simulation.Get();
}

echoes::sim::net::CommandAdmissionStatus
UEchoesSimulationSubsystem::AdmitNetworkCommand(
    const echoes::sim::net::CommandRequest& Request,
    echoes::sim::net::CommandAdmissionContext& Context,
    std::string* SimulationRejection)
{
    if (!bScenarioReady || !Simulation.IsValid())
    {
        return echoes::sim::net::CommandAdmissionStatus::InvalidSeat;
    }
    return echoes::sim::net::AdmitCommandRequest(
        Request, Context, *Simulation, SimulationRejection);
}

const echoes::sim::Entity* UEchoesSimulationSubsystem::FindEntity(uint32 EntityId) const
{
    return Simulation.IsValid() ? Simulation->FindEntity(EntityId) : nullptr;
}

AEchoesEntityView* UEchoesSimulationSubsystem::FindEntityView(uint32 EntityId) const
{
    const TWeakObjectPtr<AEchoesEntityView>* View = EntityViews.Find(EntityId);
    return View != nullptr ? View->Get() : nullptr;
}

AEchoesFogView* UEchoesSimulationSubsystem::GetFogView() const
{
    return FogView.Get();
}

AEchoesTerrainView* UEchoesSimulationSubsystem::GetTerrainView() const
{
    return TerrainView.Get();
}

FVector UEchoesSimulationSubsystem::SimToWorld(const echoes::sim::Vec2& Position) const
{
    const float MapHalfX = static_cast<float>(GetMapWidthTiles()) * 0.5f;
    const float MapHalfY = static_cast<float>(GetMapHeightTiles()) * 0.5f;
    const float TileX = static_cast<float>(Position.x.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale);
    const float TileY = static_cast<float>(Position.y.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale);
    return FVector(
        (TileX - MapHalfX) * TileWorldSize,
        (TileY - MapHalfY) * TileWorldSize,
        0.0f);
}

echoes::sim::Vec2 UEchoesSimulationSubsystem::WorldToSim(const FVector& Position) const
{
    const double MapHalfX = static_cast<double>(GetMapWidthTiles()) * 0.5;
    const double MapHalfY = static_cast<double>(GetMapHeightTiles()) * 0.5;
    const double RawX =
        (static_cast<double>(Position.X) / TileWorldSize + MapHalfX) *
        echoes::sim::kFixedScale;
    const double RawY =
        (static_cast<double>(Position.Y) / TileWorldSize + MapHalfY) *
        echoes::sim::kFixedScale;
    return echoes::sim::Vec2::FromRaw(
        FMath::RoundToInt32(RawX),
        FMath::RoundToInt32(RawY));
}

int32 UEchoesSimulationSubsystem::GetMapWidthTiles() const
{
    return Simulation.IsValid() ? Simulation->Config().mapWidthTiles
                                : PrototypeMapWidthTiles;
}

int32 UEchoesSimulationSubsystem::GetMapHeightTiles() const
{
    return Simulation.IsValid() ? Simulation->Config().mapHeightTiles
                                : PrototypeMapHeightTiles;
}
