#include "EchoesMatchReplay.h"

#include "EchoesCampaignProgress.h"
#include "EchoesCampaignTerrainBinding.h"
#include "EchoesHashUtility.h"
#include "EchoesNetworkSession.h"
#include "EchoesSkirmishSetup.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/StringBuilder.h"

#include <algorithm>
#include <tuple>

namespace
{
constexpr uint8 ReplayMagic[] = {'E', 'C', 'H', 'O', 'R', 'P', 'L', '1'};
constexpr uint8 ReplayRecordMagic[] = {'E', 'C', 'H', 'O', 'R', 'C', 'R', '1'};
constexpr uint8 CheckpointReplayMagic[] = {'E', 'C', 'H', 'O', 'R', 'C', 'P', '1'};
constexpr uint16 ReplayRecordSchemaVersion = 2;
constexpr uint16 CheckpointReplaySchemaVersion = 1;
constexpr int32 ChecksumSize = 4;
constexpr int32 MaximumReplayBytes = 256 * 1024 * 1024;
constexpr int64 MaximumReplayBrowserScanBytes = 512LL * 1024LL * 1024LL;
constexpr int32 MaximumReplayBrowserEntries = 4096;
constexpr int64 MaximumReplayBrowserNameCharacters = 1024 * 1024;
constexpr int32 MaximumIdentityBytes = 512;
constexpr int32 SerializedReplayCommandBytes = 39;

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
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Bytes.Add(static_cast<uint8>(Value >> (Index * 8)));
    }
}

void AppendU64(TArray<uint8>& Bytes, uint64 Value)
{
    for (int32 Index = 0; Index < 8; ++Index)
    {
        Bytes.Add(static_cast<uint8>(Value >> (Index * 8)));
    }
}

void AppendI32(TArray<uint8>& Bytes, int32 Value)
{
    AppendU32(Bytes, static_cast<uint32>(Value));
}

bool AppendString(TArray<uint8>& Bytes, const FString& Value, FString& OutError)
{
    FTCHARToUTF8 Encoded(*Value);
    if (Encoded.Length() < 0 || Encoded.Length() > MaximumIdentityBytes)
    {
        OutError = TEXT("Replay metadata text exceeds the bounded UTF-8 length.");
        return false;
    }
    AppendU16(Bytes, static_cast<uint16>(Encoded.Length()));
    Bytes.Append(
        reinterpret_cast<const uint8*>(Encoded.Get()), Encoded.Length());
    return true;
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
    for (int32 Index = 0; Index < 4; ++Index)
    {
        OutValue |= static_cast<uint32>(Bytes[Offset + Index]) << (Index * 8);
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
    for (int32 Index = 0; Index < 8; ++Index)
    {
        OutValue |= static_cast<uint64>(Bytes[Offset + Index]) << (Index * 8);
    }
    Offset += 8;
    return true;
}

bool ReadI32(const TArray<uint8>& Bytes, int32& Offset, int32& OutValue)
{
    uint32 Encoded = 0;
    if (!ReadU32(Bytes, Offset, Encoded))
    {
        return false;
    }
    OutValue = static_cast<int32>(Encoded);
    return true;
}

bool ReadString(
    const TArray<uint8>& Bytes,
    int32& Offset,
    FString& OutValue)
{
    uint16 Length = 0;
    if (!ReadU16(Bytes, Offset, Length) || Length > MaximumIdentityBytes ||
        Offset < 0 || Offset + Length > Bytes.Num())
    {
        return false;
    }
    const FUTF8ToTCHAR Converted(
        reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + Offset), Length);
    OutValue = FString(Converted.Length(), Converted.Get());
    Offset += Length;
    return true;
}

bool IsValidReplayId(const FString& ReplayId)
{
    if (ReplayId.IsEmpty() || ReplayId.Len() > 96)
    {
        return false;
    }
    for (const TCHAR Character : ReplayId)
    {
        if (!FChar::IsAlnum(Character) && Character != TEXT('-') &&
            Character != TEXT('_'))
        {
            return false;
        }
    }
    return true;
}

bool ReplayCommandLess(
    const echoes::sim::Command& Lhs,
    const echoes::sim::Command& Rhs)
{
    return std::tie(
               Lhs.executeTick,
               Lhs.player,
               Lhs.sequence,
               Lhs.type,
               Lhs.actor,
               Lhs.target,
               Lhs.position.x,
               Lhs.position.y,
               Lhs.buildType,
               Lhs.wellChoice,
               Lhs.warformAdaptation,
               Lhs.researchType) <
        std::tie(
               Rhs.executeTick,
               Rhs.player,
               Rhs.sequence,
               Rhs.type,
               Rhs.actor,
               Rhs.target,
               Rhs.position.x,
               Rhs.position.y,
               Rhs.buildType,
               Rhs.wellChoice,
               Rhs.warformAdaptation,
               Rhs.researchType);
}

FString ReplayRulesIdentity(const echoes::sim::Simulation& Baseline)
{
    TStringBuilder<65> Hex;
    for (const uint8 Byte : Baseline.Config().rules.contentSha256)
    {
        Hex.Appendf(TEXT("%02x"), Byte);
    }
    return Hex.ToString();
}

FString InstalledRulesIdentity()
{
    const echoes::sim::net::CompatibilityManifest Compatibility =
        echoes::network::BuildCompatibilityManifest(nullptr);
    TStringBuilder<65> Hex;
    for (const uint8 Byte : Compatibility.rulesPackSha256)
    {
        Hex.Appendf(TEXT("%02x"), Byte);
    }
    return Hex.ToString();
}

FString ContentIdentity(
    const echoes::sim::ReplayRecord& Replay,
    const FEchoesReplayCancellationCheck& ShouldCancel,
    bool& bOutCancelled)
{
    bOutCancelled = false;
    if (Replay.initialSnapshot.empty() ||
        Replay.initialSnapshot.size() > static_cast<size_t>(MaximumReplayBytes))
    {
        return {};
    }
    // This is a deterministic identity for the serialized replay baseline. It
    // provides no sender authentication; the envelope CRC likewise detects
    // accidental corruption rather than proving provenance.
    return EchoesHash::ComputeSha256Hex(
        MakeArrayView(
            Replay.initialSnapshot.data(),
            static_cast<int32>(Replay.initialSnapshot.size())),
        ShouldCancel,
        bOutCancelled);
}

uint8 CampaignOrdinalForOperation(const FString& OperationId)
{
    static constexpr const TCHAR* OperationIds[] = {
        TEXT("m01-what-the-ledger-keeps"),
        TEXT("m02-seven-accounts-of-rain"),
        TEXT("m03-a-city-on-reserve"),
        TEXT("m04-the-unburied-road"),
        TEXT("m05-terms-of-continuance"),
        TEXT("m06-names-without-births"),
        TEXT("m07-the-shape-of-silence"),
        TEXT("m08-the-shape-beside-us"),
        TEXT("m09-reserve-authority"),
        TEXT("m10-choir-at-lume-reach"),
        TEXT("m11-no-neutral-ledger"),
        TEXT("m12-the-future-that-won"),
        TEXT("m13-assembly-of-the-missing"),
        TEXT("m14-several-voices-one-command"),
        TEXT("m15-the-broken-sun"),
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(OperationIds); ++Index)
    {
        if (OperationId == OperationIds[Index])
        {
            return static_cast<uint8>(Index + 1);
        }
    }
    return 0;
}

bool ValidateOperationMap(
    const FEchoesReplayMetadata& Metadata,
    const echoes::sim::Simulation& Baseline,
    FString& OutCanonicalMapId,
    const FEchoesReplayCancellationCheck& ShouldCancel,
    FString& OutError)
{
    OutCanonicalMapId.Reset();
    const auto Cancelled = [&]()
    {
        if (ShouldCancel && ShouldCancel())
        {
            OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay scenario binding validation was cancelled.");
            return true;
        }
        return false;
    };
    if (Metadata.OperationType == EEchoesReplayOperationType::Skirmish)
    {
        EEchoesSkirmishMapPreset Preset = EEchoesSkirmishMapPreset::GlassScar;
        if (Metadata.OperationId != TEXT("skirmish"))
        {
            OutError = TEXT("Replay skirmish operation identity is unsupported.");
            return false;
        }
        if (Metadata.MapId == TEXT("glass-scar"))
        {
            Preset = EEchoesSkirmishMapPreset::GlassScar;
            OutCanonicalMapId = TEXT("glass-scar");
        }
        else if (Metadata.MapId == TEXT("crownfall-basin"))
        {
            Preset = EEchoesSkirmishMapPreset::CrownfallBasin;
            OutCanonicalMapId = TEXT("crownfall-basin");
        }
        else if (Metadata.MapId == TEXT("soryn-confluence"))
        {
            Preset = EEchoesSkirmishMapPreset::SorynConfluence;
            OutCanonicalMapId = TEXT("soryn-confluence");
        }
        else
        {
            OutError = TEXT("Replay skirmish map identity is unsupported.");
            return false;
        }
        if (Baseline.Config().mapWidthTiles !=
                FEchoesSkirmishSetupModel::MapWidthTiles ||
            Baseline.Config().mapHeightTiles !=
                FEchoesSkirmishSetupModel::MapHeightTiles)
        {
            OutError = TEXT("Replay baseline dimensions do not match its named skirmish battlefield.");
            return false;
        }
        for (int32 Y = 0; Y < FEchoesSkirmishSetupModel::MapHeightTiles; ++Y)
        {
            if (Cancelled()) return false;
            for (int32 X = 0; X < FEchoesSkirmishSetupModel::MapWidthTiles; ++X)
            {
                const echoes::sim::Terrain Expected =
                    FEchoesSkirmishSetupModel::IsBlockedTile(Preset, X, Y)
                        ? echoes::sim::Terrain::Blocked
                        : echoes::sim::Terrain::Open;
                if (Baseline.TerrainAt(X, Y) != Expected)
                {
                    OutError = TEXT("Replay baseline terrain does not match its named skirmish battlefield.");
                    return false;
                }
            }
        }
        return true;
    }
    if (Metadata.OperationType != EEchoesReplayOperationType::Campaign)
    {
        OutError = TEXT("Replay operation type is unsupported.");
        return false;
    }

    const uint8 Ordinal = CampaignOrdinalForOperation(Metadata.OperationId);
    if (Ordinal == 0)
    {
        OutError = TEXT("Replay campaign operation identity is unsupported.");
        return false;
    }
    constexpr int32 CampaignMapDimension = 64;
    if (Baseline.Config().mapWidthTiles != CampaignMapDimension ||
        Baseline.Config().mapHeightTiles != CampaignMapDimension)
    {
        OutError = TEXT("Replay baseline dimensions do not match its named campaign battlefield.");
        return false;
    }
    constexpr echoes::sim::FutureWellChoice Doctrines[] = {
        echoes::sim::FutureWellChoice::Harvest,
        echoes::sim::FutureWellChoice::Preserve,
        echoes::sim::FutureWellChoice::Reshape,
    };
    bool bNamedMapFound = false;
    for (const echoes::sim::FutureWellChoice Doctrine : Doctrines)
    {
        const echoes::world::CampaignTerrainResult Map =
            echoes::world::CheckCampaignTerrain(Ordinal, Doctrine);
        if (!Map.ok || Metadata.MapId != UTF8_TO_TCHAR(Map.map_id))
        {
            continue;
        }
        bNamedMapFound = true;
        bool bTopologyMatches = true;
        for (int32 Y = 0; Y < CampaignMapDimension && bTopologyMatches; ++Y)
        {
            if (Cancelled()) return false;
            for (int32 X = 0; X < CampaignMapDimension; ++X)
            {
                const echoes::sim::Terrain Expected =
                    echoes::world::IsCampaignTerrainPassable(
                        Ordinal, Doctrine, X, Y)
                        ? echoes::sim::Terrain::Open
                        : echoes::sim::Terrain::Blocked;
                if (Baseline.TerrainAt(X, Y) != Expected)
                {
                    bTopologyMatches = false;
                    break;
                }
            }
        }
        if (bTopologyMatches)
        {
            OutCanonicalMapId = UTF8_TO_TCHAR(Map.map_id);
            return true;
        }
    }
    OutError = bNamedMapFound
        ? TEXT("Replay baseline terrain does not match its named campaign battlefield.")
        : TEXT("Replay campaign map identity does not match its authored operation.");
    return false;
}

echoes::sim::ReplayCancellationCheck MakeCoreCancellationCheck(
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    if (!ShouldCancel)
    {
        return {};
    }
    return [&ShouldCancel]() { return ShouldCancel(); };
}

struct FReplayBaselineAuthority final
{
    std::optional<echoes::sim::Simulation> Baseline;
    FString RulesIdentity;
    FString ContentIdentity;
    FString CanonicalMapId;
};

bool LoadReplayBaselineAuthority(
    const FEchoesReplayMetadata& Metadata,
    const echoes::sim::ReplayRecord& Replay,
    FReplayBaselineAuthority& OutAuthority,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    OutAuthority = {};
    std::string BaselineError;
    OutAuthority.Baseline = echoes::sim::Simulation::LoadSnapshot(
        Replay.initialSnapshot,
        &BaselineError,
        echoes::sim::kDefaultHostilityMasks,
        MakeCoreCancellationCheck(ShouldCancel));
    if (!OutAuthority.Baseline.has_value())
    {
        OutError = BaselineError == "snapshot load cancelled"
            ? TEXT("[REPLAY_VALIDATION_CANCELLED] Replay baseline validation was cancelled.")
            : FString::Printf(
                  TEXT("Replay baseline failed to load: %s"),
                  UTF8_TO_TCHAR(BaselineError.c_str()));
        return false;
    }
    constexpr size_t MaximumReplayBaselineEntities = 4096;
    if (OutAuthority.Baseline->Entities().size() >
        MaximumReplayBaselineEntities)
    {
        OutAuthority = {};
        OutError = TEXT("Replay baseline exceeds the supported scenario complexity bound.");
        return false;
    }
    constexpr uint64 MaximumReplayBaselineVisibilityCells =
        16ULL * 1024ULL * 1024ULL;
    uint64 VisibilityCells = 0;
    for (const echoes::sim::Entity& Entity :
         OutAuthority.Baseline->Entities())
    {
        const uint64 Diameter =
            static_cast<uint64>(Entity.visionTiles) * 2ULL + 1ULL;
        VisibilityCells += Diameter * Diameter;
        if (VisibilityCells > MaximumReplayBaselineVisibilityCells)
        {
            OutAuthority = {};
            OutError = TEXT("Replay baseline exceeds the supported visibility-work bound.");
            return false;
        }
    }
    if (!ValidateOperationMap(
            Metadata,
            *OutAuthority.Baseline,
            OutAuthority.CanonicalMapId,
            ShouldCancel,
            OutError))
    {
        OutAuthority = {};
        return false;
    }
    OutAuthority.RulesIdentity = ReplayRulesIdentity(*OutAuthority.Baseline);
    if (OutAuthority.RulesIdentity != InstalledRulesIdentity())
    {
        OutAuthority = {};
        OutError = TEXT("Replay rules identity is not installed in this runtime.");
        return false;
    }
    bool bHashCancelled = false;
    OutAuthority.ContentIdentity = ContentIdentity(
        Replay, ShouldCancel, bHashCancelled);
    if (bHashCancelled)
    {
        OutAuthority = {};
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay content identity validation was cancelled.");
        return false;
    }
    if (OutAuthority.RulesIdentity.IsEmpty() ||
        OutAuthority.ContentIdentity.IsEmpty())
    {
        OutAuthority = {};
        OutError = TEXT("Replay baseline cannot provide authoritative rules and content identities.");
        return false;
    }
    return true;
}

bool LoadBoundedReplayFile(
    const FString& Path,
    int64 RemainingScanBytes,
    TArray<uint8>& OutBytes,
    int64& OutFileSize,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    OutBytes.Reset();
    OutFileSize = 0;
    OutError.Reset();
    IPlatformFile& PlatformFile =
        FPlatformFileManager::Get().GetPlatformFile();
    TUniquePtr<IFileHandle> Handle(PlatformFile.OpenRead(*Path));
    if (!Handle)
    {
        OutError = TEXT("Replay file could not be opened.");
        return false;
    }
    const int64 Size = Handle->Size();
    const int64 MinimumSize =
        static_cast<int64>(UE_ARRAY_COUNT(ReplayMagic)) + 2 + ChecksumSize;
    if (Size < MinimumSize || Size > MaximumReplayBytes)
    {
        OutError = TEXT("Replay envelope size is invalid.");
        return false;
    }
    if (Size > RemainingScanBytes)
    {
        OutError = TEXT("Replay browser aggregate read bound was reached.");
        return false;
    }
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay file read was cancelled.");
        return false;
    }
    OutBytes.SetNumUninitialized(static_cast<int32>(Size));
    constexpr int64 ReadChunkBytes = 1024LL * 1024LL;
    int64 ReadOffset = 0;
    while (ReadOffset < Size)
    {
        if (ShouldCancel && ShouldCancel())
        {
            OutBytes.Reset();
            OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay file read was cancelled.");
            return false;
        }
        const int64 ThisRead = FMath::Min(ReadChunkBytes, Size - ReadOffset);
        if (!Handle->Read(OutBytes.GetData() + ReadOffset, ThisRead))
        {
            OutBytes.Reset();
            OutError = TEXT("Replay file could not be read completely.");
            return false;
        }
        ReadOffset += ThisRead;
    }
    OutFileSize = Size;
    return true;
}

bool AtomicReplace(const FString& Destination, const FString& Source)
{
#if PLATFORM_MAC
    return FPlatformFileManager::Get().GetPlatformFile().MoveFile(
        *Destination, *Source);
#else
    return IFileManager::Get().Move(
        *Destination, *Source, true, true, true, true);
#endif
}

bool ValidateContainerChecksum(
    const TArray<uint8>& Bytes,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel = {})
{
    if (Bytes.Num() < static_cast<int32>(UE_ARRAY_COUNT(ReplayMagic)) +
            2 + ChecksumSize ||
        Bytes.Num() > MaximumReplayBytes)
    {
        OutError = TEXT("Replay envelope size is invalid.");
        return false;
    }
    int32 ChecksumOffset = Bytes.Num() - ChecksumSize;
    uint32 Stored = 0;
    if (!ReadU32(Bytes, ChecksumOffset, Stored))
    {
        OutError = TEXT("Replay envelope checksum does not match.");
        return false;
    }
    constexpr int32 CrcChunkBytes = 1024 * 1024;
    uint32 Actual = 0;
    int32 CrcOffset = 0;
    const int32 PayloadBytes = Bytes.Num() - ChecksumSize;
    while (CrcOffset < PayloadBytes)
    {
        if (ShouldCancel && ShouldCancel())
        {
            OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay checksum validation was cancelled.");
            return false;
        }
        const int32 ThisChunk = FMath::Min(
            CrcChunkBytes, PayloadBytes - CrcOffset);
        Actual = FCrc::MemCrc32(
            Bytes.GetData() + CrcOffset, ThisChunk, Actual);
        CrcOffset += ThisChunk;
    }
    if (Stored != Actual)
    {
        OutError = TEXT("Replay envelope checksum does not match.");
        return false;
    }
    return true;
}

bool HasCheckpointReplayMagic(const TArray<uint8>& CandidatePayload)
{
    if (CandidatePayload.Num() <
        static_cast<int32>(UE_ARRAY_COUNT(CheckpointReplayMagic)))
    {
        return false;
    }
    for (int32 Index = 0;
         Index < static_cast<int32>(UE_ARRAY_COUNT(CheckpointReplayMagic));
         ++Index)
    {
        if (CandidatePayload[Index] != CheckpointReplayMagic[Index])
        {
            return false;
        }
    }
    return true;
}

bool ExtractCheckpointBindingSlices(
    const TArray<uint8>& CandidatePayload,
    TArray<uint8>& OutCheckpointPayload,
    TArray<uint8>& OutReplayBytes,
    FString& OutError)
{
    OutCheckpointPayload.Reset();
    OutReplayBytes.Reset();
    if (!HasCheckpointReplayMagic(CandidatePayload))
    {
        OutError = TEXT("Generated checkpoint is not replay-bound.");
        return false;
    }
    if (!ValidateContainerChecksum(CandidatePayload, OutError))
    {
        OutError = TEXT("Replay-bound checkpoint checksum does not match.");
        return false;
    }
    int32 Offset = UE_ARRAY_COUNT(CheckpointReplayMagic);
    uint16 Version = 0;
    uint32 CheckpointLength = 0;
    uint32 ReplayLength = 0;
    const int32 PayloadEnd = CandidatePayload.Num() - ChecksumSize;
    if (!ReadU16(CandidatePayload, Offset, Version) ||
        Version != CheckpointReplaySchemaVersion ||
        !ReadU32(CandidatePayload, Offset, CheckpointLength) ||
        !ReadU32(CandidatePayload, Offset, ReplayLength) ||
        static_cast<uint64>(CheckpointLength) + ReplayLength !=
            static_cast<uint64>(PayloadEnd - Offset))
    {
        OutError = TEXT("Replay-bound checkpoint lengths are invalid.");
        return false;
    }
    OutCheckpointPayload = TArray<uint8>(
        CandidatePayload.GetData() + Offset,
        static_cast<int32>(CheckpointLength));
    Offset += static_cast<int32>(CheckpointLength);
    OutReplayBytes = TArray<uint8>(
        CandidatePayload.GetData() + Offset,
        static_cast<int32>(ReplayLength));
    return true;
}

bool ReadMetadata(
    const TArray<uint8>& Bytes,
    int32& Offset,
    FEchoesReplayMetadata& OutMetadata,
    FString& OutError)
{
    for (const uint8 Expected : ReplayMagic)
    {
        uint8 Actual = 0;
        if (!ReadU8(Bytes, Offset, Actual) || Actual != Expected)
        {
            OutError = TEXT("Replay envelope magic is invalid.");
            return false;
        }
    }
    uint16 Version = 0;
    uint64 RecordedTicks = 0;
    uint8 Operation = 0;
    uint8 Completed = 0;
    uint8 Outcome = 0;
    uint8 OperationResult = 0;
    uint8 OutcomeCause = 0;
    uint8 FactionCount = 0;
    if (!ReadU16(Bytes, Offset, Version) ||
        Version != FEchoesMatchReplayStore::SchemaVersion ||
        !ReadString(Bytes, Offset, OutMetadata.ReplayId) ||
        !ReadString(Bytes, Offset, OutMetadata.MapId) ||
        !ReadString(Bytes, Offset, OutMetadata.OperationId) ||
        !ReadString(Bytes, Offset, OutMetadata.BuildIdentity) ||
        !ReadString(Bytes, Offset, OutMetadata.RulesIdentity) ||
        !ReadString(Bytes, Offset, OutMetadata.ContentIdentity) ||
        !ReadU64(Bytes, Offset, RecordedTicks) ||
        !ReadU8(Bytes, Offset, Operation) ||
        !ReadU8(Bytes, Offset, Completed) ||
        !ReadU64(Bytes, Offset, OutMetadata.CoverageStartTick) ||
        !ReadU64(Bytes, Offset, OutMetadata.DurationTicks) ||
        !ReadU64(Bytes, Offset, OutMetadata.StatisticsCoverageTicks) ||
        !ReadU8(Bytes, Offset, Outcome) ||
        !ReadU8(Bytes, Offset, OperationResult) ||
        !ReadU8(Bytes, Offset, OutcomeCause) ||
        !ReadString(Bytes, Offset, OutMetadata.OutcomeReasonId) ||
        !ReadString(Bytes, Offset, OutMetadata.IrreversibleRecordId) ||
        !ReadU64(Bytes, Offset, OutMetadata.FinalChecksum) ||
        !ReadU8(Bytes, Offset, FactionCount) || FactionCount > 4 ||
        Operation > static_cast<uint8>(EEchoesReplayOperationType::Campaign) ||
        Completed > 1 ||
        Outcome > static_cast<uint8>(echoes::sim::MatchOutcome::Player3Victory) ||
        OperationResult > static_cast<uint8>(EEchoesReplayOperationResult::CampaignFailure) ||
        OutcomeCause > static_cast<uint8>(EEchoesReplayOutcomeCause::CampaignFailurePredicate) ||
        RecordedTicks > static_cast<uint64>(FDateTime::MaxValue().GetTicks()))
    {
        OutError = TEXT("Replay metadata is malformed or unsupported.");
        return false;
    }
    OutMetadata.RecordedUtc = FDateTime(static_cast<int64>(RecordedTicks));
    OutMetadata.OperationType = static_cast<EEchoesReplayOperationType>(Operation);
    OutMetadata.bOperationCompleted = Completed != 0;
    OutMetadata.Outcome = static_cast<echoes::sim::MatchOutcome>(Outcome);
    OutMetadata.OperationResult =
        static_cast<EEchoesReplayOperationResult>(OperationResult);
    OutMetadata.OutcomeCause =
        static_cast<EEchoesReplayOutcomeCause>(OutcomeCause);
    OutMetadata.PlayerFactions.Reset(FactionCount);
    for (uint8 Index = 0; Index < FactionCount; ++Index)
    {
        uint8 Faction = 0;
        if (!ReadU8(Bytes, Offset, Faction) ||
            Faction > static_cast<uint8>(echoes::sim::Faction::HollowChoir))
        {
            OutError = TEXT("Replay player faction metadata is invalid.");
            return false;
        }
        OutMetadata.PlayerFactions.Add(
            static_cast<echoes::sim::Faction>(Faction));
    }
    return true;
}
}

uint64 FEchoesReplayResultAuthority::BeginResult()
{
    ++Generation;
    if (Generation == 0)
    {
        ++Generation;
    }
    State = EEchoesReplayArchiveState::Pending;
    Error.Reset();
    Completed.Reset();
    return Generation;
}

bool FEchoesReplayResultAuthority::Publish(
    FEchoesReplayArchiveResult&& Result)
{
    if (Result.Generation == 0 || Result.Generation != Generation)
    {
        return false;
    }
    State = Result.bSucceeded
        ? EEchoesReplayArchiveState::Succeeded
        : EEchoesReplayArchiveState::Failed;
    Error = MoveTemp(Result.Error);
    if (Result.bFinalized)
    {
        Completed = MoveTemp(Result.Envelope);
    }
    else
    {
        Completed.Reset();
    }
    return true;
}

FString FEchoesMatchReplayStore::GetReplayDirectory()
{
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("Replays"));
}

bool FEchoesMatchReplayStore::EncodeReplayRecord(
    const echoes::sim::ReplayRecord& Replay,
    TArray<uint8>& OutBytes,
    FString& OutError)
{
    OutBytes.Reset();
    OutError.Reset();
    std::string ValidationError;
    if (!echoes::sim::Simulation::ReplayToEnd(
            Replay, &ValidationError).has_value())
    {
        OutError = FString::Printf(
            TEXT("Replay prefix validation failed: %s"),
            UTF8_TO_TCHAR(ValidationError.c_str()));
        return false;
    }
    if (Replay.initialSnapshot.size() > static_cast<size_t>(MAX_uint32) ||
        Replay.commands.size() > echoes::sim::kMaximumCommandLogEntries)
    {
        OutError = TEXT("Replay prefix exceeds serialized bounds.");
        return false;
    }
    OutBytes.Append(ReplayRecordMagic, UE_ARRAY_COUNT(ReplayRecordMagic));
    AppendU16(OutBytes, ReplayRecordSchemaVersion);
    AppendU32(OutBytes, Replay.version);
    AppendU64(OutBytes, Replay.finalTick);
    AppendU64(OutBytes, Replay.finalChecksum);
    AppendU8(OutBytes, Replay.forfeitingPlayer);
    AppendU32(OutBytes, static_cast<uint32>(Replay.initialSnapshot.size()));
    AppendU32(OutBytes, static_cast<uint32>(Replay.commands.size()));
    OutBytes.Append(
        Replay.initialSnapshot.data(),
        static_cast<int32>(Replay.initialSnapshot.size()));
    for (const echoes::sim::Command& Command : Replay.commands)
    {
        AppendU64(OutBytes, Command.executeTick);
        AppendU8(OutBytes, Command.player);
        AppendU64(OutBytes, Command.sequence);
        AppendU8(OutBytes, static_cast<uint8>(Command.type));
        AppendU32(OutBytes, Command.actor);
        AppendU32(OutBytes, Command.target);
        AppendI32(OutBytes, Command.position.x.Raw());
        AppendI32(OutBytes, Command.position.y.Raw());
        AppendU8(OutBytes, static_cast<uint8>(Command.buildType));
        AppendU8(OutBytes, static_cast<uint8>(Command.wellChoice));
        AppendU8(OutBytes, static_cast<uint8>(Command.warformAdaptation));
        AppendU8(OutBytes, static_cast<uint8>(Command.researchType));
        AppendU8(OutBytes, Command.queue ? 1 : 0);
    }
    if (OutBytes.Num() > MaximumReplayBytes - ChecksumSize)
    {
        OutBytes.Reset();
        OutError = TEXT("Replay prefix exceeds the file-size bound.");
        return false;
    }
    AppendU32(OutBytes, FCrc::MemCrc32(OutBytes.GetData(), OutBytes.Num()));
    return true;
}

bool FEchoesMatchReplayStore::DecodeReplayRecord(
    const TArray<uint8>& Bytes,
    echoes::sim::ReplayRecord& OutReplay,
    FString& OutError)
{
    OutReplay = {};
    OutError.Reset();
    if (!ValidateContainerChecksum(Bytes, OutError))
    {
        return false;
    }
    int32 Offset = 0;
    for (const uint8 Expected : ReplayRecordMagic)
    {
        uint8 Actual = 0;
        if (!ReadU8(Bytes, Offset, Actual) || Actual != Expected)
        {
            OutError = TEXT("Replay prefix magic is invalid.");
            return false;
        }
    }
    uint16 EnvelopeVersion = 0;
    uint32 ReplayVersion = 0;
    uint64 FinalTick = 0;
    uint64 FinalChecksum = 0;
    uint8 ForfeitingPlayer = echoes::sim::kNeutralPlayer;
    uint32 SnapshotLength = 0;
    uint32 CommandCount = 0;
    const int32 PayloadEnd = Bytes.Num() - ChecksumSize;
    if (!ReadU16(Bytes, Offset, EnvelopeVersion))
    {
        OutError = TEXT("Replay prefix envelope version cannot be read.");
        return false;
    }
    if (EnvelopeVersion != ReplayRecordSchemaVersion)
    {
        OutError = FString::Printf(
            TEXT("Replay prefix envelope version %u does not match schema %u."),
            EnvelopeVersion, ReplayRecordSchemaVersion);
        return false;
    }
    if (!ReadU32(Bytes, Offset, ReplayVersion))
    {
        OutError = TEXT("Replay prefix version cannot be read.");
        return false;
    }
    if (!ReadU64(Bytes, Offset, FinalTick) ||
        !ReadU64(Bytes, Offset, FinalChecksum) ||
        !ReadU8(Bytes, Offset, ForfeitingPlayer))
    {
        OutError = TEXT("Replay prefix header fields cannot be read.");
        return false;
    }
    if (ReplayVersion == echoes::sim::kLegacyReplayVersion &&
        ForfeitingPlayer != echoes::sim::kNeutralPlayer)
    {
        OutError = TEXT("Legacy v24 replay cannot contain a forfeit marker.");
        return false;
    }
    if (!ReadU32(Bytes, Offset, SnapshotLength) ||
        !ReadU32(Bytes, Offset, CommandCount))
    {
        OutError = TEXT("Replay prefix lengths cannot be read.");
        return false;
    }
    if (CommandCount > echoes::sim::kMaximumCommandLogEntries)
    {
        OutError = FString::Printf(
            TEXT("Replay prefix command count %u exceeds bound."), CommandCount);
        return false;
    }
    if (SnapshotLength > static_cast<uint32>(PayloadEnd - Offset))
    {
        OutError = FString::Printf(
            TEXT("Replay prefix snapshot length %u exceeds remaining payload %d."),
            SnapshotLength, PayloadEnd - Offset);
        return false;
    }
    if (static_cast<uint64>(SnapshotLength) +
            static_cast<uint64>(CommandCount) * SerializedReplayCommandBytes !=
        static_cast<uint64>(PayloadEnd - Offset))
    {
        OutError = FString::Printf(
            TEXT("Replay prefix payload lengths mismatch: snapshot=%u commands=%u (%u bytes) remaining=%d."),
            SnapshotLength, CommandCount, CommandCount * SerializedReplayCommandBytes, PayloadEnd - Offset);
        return false;
    }
    OutReplay.version = ReplayVersion;
    OutReplay.finalTick = FinalTick;
    OutReplay.finalChecksum = FinalChecksum;
    OutReplay.forfeitingPlayer = ForfeitingPlayer;
    OutReplay.initialSnapshot.assign(
        Bytes.GetData() + Offset, Bytes.GetData() + Offset + SnapshotLength);
    Offset += static_cast<int32>(SnapshotLength);
    OutReplay.commands.resize(CommandCount);
    for (echoes::sim::Command& Command : OutReplay.commands)
    {
        uint8 Type = 0;
        uint8 BuildType = 0;
        uint8 WellChoice = 0;
        uint8 Adaptation = 0;
        uint8 Research = 0;
        uint8 Queued = 0;
        int32 RawX = 0;
        int32 RawY = 0;
        if (!ReadU64(Bytes, Offset, Command.executeTick) ||
            !ReadU8(Bytes, Offset, Command.player) ||
            !ReadU64(Bytes, Offset, Command.sequence) ||
            !ReadU8(Bytes, Offset, Type) ||
            !ReadU32(Bytes, Offset, Command.actor) ||
            !ReadU32(Bytes, Offset, Command.target) ||
            !ReadI32(Bytes, Offset, RawX) || !ReadI32(Bytes, Offset, RawY) ||
            !ReadU8(Bytes, Offset, BuildType) ||
            !ReadU8(Bytes, Offset, WellChoice) ||
            !ReadU8(Bytes, Offset, Adaptation) ||
            !ReadU8(Bytes, Offset, Research) ||
            !ReadU8(Bytes, Offset, Queued) || Queued > 1)
        {
            OutReplay = {};
            OutError = TEXT("Replay prefix command payload is truncated.");
            return false;
        }
        Command.type = static_cast<echoes::sim::CommandType>(Type);
        Command.position = echoes::sim::Vec2::FromRaw(RawX, RawY);
        Command.buildType = static_cast<echoes::sim::EntityType>(BuildType);
        Command.wellChoice = static_cast<echoes::sim::FutureWellChoice>(WellChoice);
        Command.warformAdaptation =
            static_cast<echoes::sim::WarformAdaptation>(Adaptation);
        Command.researchType = static_cast<echoes::sim::ResearchType>(Research);
        Command.queue = Queued != 0;
    }
    std::string ValidationError;
    if (Offset != PayloadEnd ||
        !echoes::sim::Simulation::ReplayToEnd(
            OutReplay, &ValidationError).has_value())
    {
        OutReplay = {};
        if (Offset != PayloadEnd)
        {
            OutError = TEXT("Replay prefix contains trailing payload data.");
        }
        else
        {
            OutError = FString::Printf(
                TEXT("Replay prefix authority validation failed: %s"),
                UTF8_TO_TCHAR(ValidationError.c_str()));
        }
        return false;
    }
    return true;
}

bool FEchoesMatchReplayStore::BindCheckpointPayload(
    const TArray<uint8>& CheckpointPayload,
    const echoes::sim::ReplayRecord& ReplayPrefix,
    TArray<uint8>& OutBoundPayload,
    FString& OutError)
{
    TArray<uint8> ValidatedReplayBytes;
    return BindCheckpointPayload(
        CheckpointPayload,
        ReplayPrefix,
        OutBoundPayload,
        ValidatedReplayBytes,
        OutError);
}

bool FEchoesMatchReplayStore::BindCheckpointPayload(
    const TArray<uint8>& CheckpointPayload,
    const echoes::sim::ReplayRecord& ReplayPrefix,
    TArray<uint8>& OutBoundPayload,
    TArray<uint8>& OutValidatedReplayBytes,
    FString& OutError)
{
    OutBoundPayload.Reset();
    OutValidatedReplayBytes.Reset();
    OutError.Reset();
    TArray<uint8> ReplayBytes;
    if (!EncodeReplayRecord(ReplayPrefix, ReplayBytes, OutError))
    {
        return false;
    }
    if (CheckpointPayload.Num() < 0 ||
        static_cast<uint64>(CheckpointPayload.Num()) +
                static_cast<uint64>(ReplayBytes.Num()) +
                UE_ARRAY_COUNT(CheckpointReplayMagic) + 2U + 8U +
                ChecksumSize >
            static_cast<uint64>(MaximumReplayBytes))
    {
        OutError = TEXT("Replay-bound checkpoint exceeds the file-size bound.");
        return false;
    }
    OutBoundPayload.Append(
        CheckpointReplayMagic, UE_ARRAY_COUNT(CheckpointReplayMagic));
    AppendU16(OutBoundPayload, CheckpointReplaySchemaVersion);
    AppendU32(OutBoundPayload, static_cast<uint32>(CheckpointPayload.Num()));
    AppendU32(OutBoundPayload, static_cast<uint32>(ReplayBytes.Num()));
    OutBoundPayload.Append(CheckpointPayload);
    OutBoundPayload.Append(ReplayBytes);
    AppendU32(
        OutBoundPayload,
        FCrc::MemCrc32(OutBoundPayload.GetData(), OutBoundPayload.Num()));
    OutValidatedReplayBytes = MoveTemp(ReplayBytes);
    return true;
}

bool FEchoesMatchReplayStore::ExtractGeneratedCheckpointPayload(
    const TArray<uint8>& CandidatePayload,
    const TArray<uint8>& ExpectedReplayBytes,
    TArray<uint8>& OutCheckpointPayload,
    FString& OutError)
{
    OutCheckpointPayload.Reset();
    OutError.Reset();
    if (ExpectedReplayBytes.IsEmpty())
    {
        OutError = TEXT("Generated checkpoint replay proof is empty.");
        return false;
    }
    TArray<uint8> EmbeddedReplayBytes;
    if (!ExtractCheckpointBindingSlices(
            CandidatePayload,
            OutCheckpointPayload,
            EmbeddedReplayBytes,
            OutError))
    {
        return false;
    }
    if (EmbeddedReplayBytes != ExpectedReplayBytes)
    {
        OutCheckpointPayload.Reset();
        OutError = TEXT("Generated checkpoint replay proof does not match the validated replay bytes.");
        return false;
    }
    return true;
}

EEchoesCheckpointReplayBindingRead
FEchoesMatchReplayStore::ExtractCheckpointPayload(
    const TArray<uint8>& CandidatePayload,
    TArray<uint8>& OutCheckpointPayload,
    echoes::sim::ReplayRecord& OutReplayPrefix,
    FString& OutError)
{
    OutReplayPrefix = {};
    OutError.Reset();
    if (!HasCheckpointReplayMagic(CandidatePayload))
    {
        OutCheckpointPayload = CandidatePayload;
        return EEchoesCheckpointReplayBindingRead::LegacyUnbound;
    }
    TArray<uint8> ReplayBytes;
    if (!ExtractCheckpointBindingSlices(
            CandidatePayload,
            OutCheckpointPayload,
            ReplayBytes,
            OutError))
    {
        return EEchoesCheckpointReplayBindingRead::Invalid;
    }
    if (!DecodeReplayRecord(ReplayBytes, OutReplayPrefix, OutError))
    {
        OutCheckpointPayload.Reset();
        OutReplayPrefix = {};
        OutError = FString::Printf(
            TEXT("Replay-bound checkpoint prefix is invalid: %s"), *OutError);
        return EEchoesCheckpointReplayBindingRead::Invalid;
    }
    return EEchoesCheckpointReplayBindingRead::Bound;
}

bool FEchoesMatchReplayStore::FinalizeEnvelope(
    const FEchoesReplayMetadata& RequestedMetadata,
    const echoes::sim::ReplayRecord& Replay,
    FEchoesReplayEnvelope& OutEnvelope,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    OutError.Reset();
    OutEnvelope = {};
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled.");
        return false;
    }
    if (RequestedMetadata.MapId.IsEmpty() ||
        RequestedMetadata.OperationId.IsEmpty() ||
        RequestedMetadata.BuildIdentity.IsEmpty() ||
        RequestedMetadata.RecordedUtc == FDateTime::MinValue() ||
        !RequestedMetadata.bOperationCompleted)
    {
        OutError = TEXT("Completed replay metadata requires map, operation, build, UTC date, and completion evidence.");
        return false;
    }
    if (RequestedMetadata.OperationType == EEchoesReplayOperationType::Campaign)
    {
        const bool bCoherentCampaignResult =
            (RequestedMetadata.OperationResult ==
                 EEchoesReplayOperationResult::CampaignSuccess &&
             RequestedMetadata.OutcomeCause ==
                 EEchoesReplayOutcomeCause::CampaignObjectivesComplete) ||
            (RequestedMetadata.OperationResult ==
                 EEchoesReplayOperationResult::CampaignFailure &&
             RequestedMetadata.OutcomeCause ==
                 EEchoesReplayOutcomeCause::CampaignFailurePredicate);
        if (!bCoherentCampaignResult ||
            RequestedMetadata.OutcomeReasonId.IsEmpty())
        {
            OutError = TEXT("Campaign replay result, cause, and reason must form a coherent completed outcome.");
            return false;
        }
    }
    FReplayBaselineAuthority BaselineAuthority;
    if (!LoadReplayBaselineAuthority(
            RequestedMetadata,
            Replay,
            BaselineAuthority,
            OutError,
            ShouldCancel))
    {
        return false;
    }
    // The baseline has established metadata authority. Release it before the
    // report reconstructs its own detached simulation.
    BaselineAuthority.Baseline.reset();
    std::string ReportError;
    std::optional<echoes::sim::MatchReport> Report =
        echoes::sim::Simulation::BuildMatchReport(
            Replay,
            &ReportError,
            MakeCoreCancellationCheck(ShouldCancel));
    if (!Report.has_value())
    {
        OutError = ReportError == "replay validation cancelled"
            ? TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled before authority could be established.")
            : FString::Printf(
                  TEXT("Replay authority rejected the recording: %s"),
                  UTF8_TO_TCHAR(ReportError.c_str()));
        return false;
    }
    if (RequestedMetadata.OperationType == EEchoesReplayOperationType::Skirmish &&
        Report->outcome == echoes::sim::MatchOutcome::Ongoing)
    {
        OutError = TEXT("A completed skirmish replay requires a terminal core outcome.");
        return false;
    }

    OutEnvelope.Metadata = RequestedMetadata;
    OutEnvelope.Metadata.MapId = BaselineAuthority.CanonicalMapId;
    OutEnvelope.Metadata.CoverageStartTick = Report->baselineTick;
    OutEnvelope.Metadata.DurationTicks = Report->finalTick;
    OutEnvelope.Metadata.StatisticsCoverageTicks = Report->durationTicks;
    OutEnvelope.Metadata.Outcome = Report->outcome;
    OutEnvelope.Metadata.FinalChecksum = Report->finalChecksum;
    OutEnvelope.Metadata.RulesIdentity = BaselineAuthority.RulesIdentity;
    OutEnvelope.Metadata.ContentIdentity = BaselineAuthority.ContentIdentity;
    if (RequestedMetadata.OperationType == EEchoesReplayOperationType::Skirmish)
    {
        OutEnvelope.Metadata.OperationResult =
            static_cast<EEchoesReplayOperationResult>(Report->outcome);
        OutEnvelope.Metadata.OutcomeCause =
            Report->outcomeCause == echoes::sim::MatchOutcomeCause::PlayerForfeit
                ? EEchoesReplayOutcomeCause::PlayerForfeit
                : EEchoesReplayOutcomeCause::CommandCoreLoss;
        OutEnvelope.Metadata.OutcomeReasonId =
            Report->outcomeCause == echoes::sim::MatchOutcomeCause::PlayerForfeit
                ? TEXT("player_forfeit")
                : TEXT("command_core_loss");
    }
    OutEnvelope.Metadata.PlayerFactions.Reset();
    for (const echoes::sim::MatchPlayerStatistics& Player : Report->players)
    {
        if (Player.active)
        {
            OutEnvelope.Metadata.PlayerFactions.Add(Player.faction);
        }
    }
    if (OutEnvelope.Metadata.ReplayId.IsEmpty())
    {
        OutEnvelope.Metadata.ReplayId = FString::Printf(
            TEXT("%s-%016llx"),
            *RequestedMetadata.RecordedUtc.ToString(TEXT("%Y%m%dT%H%M%SZ")),
            static_cast<unsigned long long>(Report->finalChecksum));
    }
    if (!IsValidReplayId(OutEnvelope.Metadata.ReplayId))
    {
        OutError = TEXT("Replay ID contains unsupported filename characters.");
        OutEnvelope = {};
        return false;
    }
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled before publication.");
        OutEnvelope = {};
        return false;
    }
    OutEnvelope.Replay = Replay;
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled before publication.");
        OutEnvelope = {};
        return false;
    }
    OutEnvelope.Report = MoveTemp(*Report);
    return true;
}

bool FEchoesMatchReplayStore::Encode(
    const FEchoesReplayEnvelope& Envelope,
    TArray<uint8>& OutBytes,
    FString& OutError)
{
    OutBytes.Reset();
    FEchoesReplayEnvelope Validated;
    if (!FinalizeEnvelope(
            Envelope.Metadata, Envelope.Replay, Validated, OutError))
    {
        return false;
    }
    const FEchoesReplayMetadata& Metadata = Validated.Metadata;
    OutBytes.Append(ReplayMagic, UE_ARRAY_COUNT(ReplayMagic));
    AppendU16(OutBytes, SchemaVersion);
    if (!AppendString(OutBytes, Metadata.ReplayId, OutError) ||
        !AppendString(OutBytes, Metadata.MapId, OutError) ||
        !AppendString(OutBytes, Metadata.OperationId, OutError) ||
        !AppendString(OutBytes, Metadata.BuildIdentity, OutError) ||
        !AppendString(OutBytes, Metadata.RulesIdentity, OutError) ||
        !AppendString(OutBytes, Metadata.ContentIdentity, OutError))
    {
        OutBytes.Reset();
        return false;
    }
    AppendU64(OutBytes, static_cast<uint64>(Metadata.RecordedUtc.GetTicks()));
    AppendU8(OutBytes, static_cast<uint8>(Metadata.OperationType));
    AppendU8(OutBytes, Metadata.bOperationCompleted ? 1 : 0);
    AppendU64(OutBytes, Metadata.CoverageStartTick);
    AppendU64(OutBytes, Metadata.DurationTicks);
    AppendU64(OutBytes, Metadata.StatisticsCoverageTicks);
    AppendU8(OutBytes, static_cast<uint8>(Metadata.Outcome));
    AppendU8(OutBytes, static_cast<uint8>(Metadata.OperationResult));
    AppendU8(OutBytes, static_cast<uint8>(Metadata.OutcomeCause));
    if (!AppendString(OutBytes, Metadata.OutcomeReasonId, OutError) ||
        !AppendString(OutBytes, Metadata.IrreversibleRecordId, OutError))
    {
        OutBytes.Reset();
        return false;
    }
    AppendU64(OutBytes, Metadata.FinalChecksum);
    AppendU8(OutBytes, static_cast<uint8>(Metadata.PlayerFactions.Num()));
    for (const echoes::sim::Faction Faction : Metadata.PlayerFactions)
    {
        AppendU8(OutBytes, static_cast<uint8>(Faction));
    }

    const echoes::sim::ReplayRecord& Replay = Validated.Replay;
    if (Replay.initialSnapshot.size() > static_cast<size_t>(MAX_uint32) ||
        Replay.commands.size() > echoes::sim::kMaximumCommandLogEntries)
    {
        OutError = TEXT("Replay payload exceeds serialized bounds.");
        OutBytes.Reset();
        return false;
    }
    AppendU32(OutBytes, Replay.version);
    AppendU64(OutBytes, Replay.finalTick);
    AppendU64(OutBytes, Replay.finalChecksum);
    AppendU8(OutBytes, Replay.forfeitingPlayer);
    AppendU32(OutBytes, static_cast<uint32>(Replay.initialSnapshot.size()));
    AppendU32(OutBytes, static_cast<uint32>(Replay.commands.size()));
    OutBytes.Append(
        Replay.initialSnapshot.data(),
        static_cast<int32>(Replay.initialSnapshot.size()));
    for (const echoes::sim::Command& Command : Replay.commands)
    {
        AppendU64(OutBytes, Command.executeTick);
        AppendU8(OutBytes, Command.player);
        AppendU64(OutBytes, Command.sequence);
        AppendU8(OutBytes, static_cast<uint8>(Command.type));
        AppendU32(OutBytes, Command.actor);
        AppendU32(OutBytes, Command.target);
        AppendI32(OutBytes, Command.position.x.Raw());
        AppendI32(OutBytes, Command.position.y.Raw());
        AppendU8(OutBytes, static_cast<uint8>(Command.buildType));
        AppendU8(OutBytes, static_cast<uint8>(Command.wellChoice));
        AppendU8(OutBytes, static_cast<uint8>(Command.warformAdaptation));
        AppendU8(OutBytes, static_cast<uint8>(Command.researchType));
        AppendU8(OutBytes, Command.queue ? 1 : 0);
    }
    if (OutBytes.Num() > MaximumReplayBytes - ChecksumSize)
    {
        OutError = TEXT("Replay envelope exceeds the file-size bound.");
        OutBytes.Reset();
        return false;
    }
    AppendU32(OutBytes, FCrc::MemCrc32(OutBytes.GetData(), OutBytes.Num()));
    return true;
}

bool FEchoesMatchReplayStore::Decode(
    const TArray<uint8>& Bytes,
    FEchoesReplayEnvelope& OutEnvelope,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    OutEnvelope = {};
    OutError.Reset();
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled.");
        return false;
    }
    if (!ValidateContainerChecksum(Bytes, OutError, ShouldCancel))
    {
        return false;
    }
    int32 Offset = 0;
    FEchoesReplayMetadata StoredMetadata;
    if (!ReadMetadata(Bytes, Offset, StoredMetadata, OutError))
    {
        return false;
    }
    uint32 ReplayVersion = 0;
    uint64 FinalTick = 0;
    uint64 FinalChecksum = 0;
    uint8 ForfeitingPlayer = echoes::sim::kNeutralPlayer;
    uint32 SnapshotLength = 0;
    uint32 CommandCount = 0;
    const int32 PayloadEnd = Bytes.Num() - ChecksumSize;
    if (!ReadU32(Bytes, Offset, ReplayVersion) ||
        !ReadU64(Bytes, Offset, FinalTick) ||
        !ReadU64(Bytes, Offset, FinalChecksum) ||
        !ReadU8(Bytes, Offset, ForfeitingPlayer) ||
        !ReadU32(Bytes, Offset, SnapshotLength) ||
        !ReadU32(Bytes, Offset, CommandCount) ||
        CommandCount > echoes::sim::kMaximumCommandLogEntries ||
        SnapshotLength > static_cast<uint32>(PayloadEnd - Offset) ||
        static_cast<uint64>(SnapshotLength) +
                static_cast<uint64>(CommandCount) * SerializedReplayCommandBytes !=
            static_cast<uint64>(PayloadEnd - Offset))
    {
        OutError = TEXT("Replay payload lengths are invalid.");
        return false;
    }
    echoes::sim::ReplayRecord Replay;
    Replay.version = ReplayVersion;
    Replay.finalTick = FinalTick;
    Replay.finalChecksum = FinalChecksum;
    Replay.forfeitingPlayer = ForfeitingPlayer;
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay payload materialization was cancelled.");
        return false;
    }
    Replay.initialSnapshot.assign(
        Bytes.GetData() + Offset, Bytes.GetData() + Offset + SnapshotLength);
    Offset += static_cast<int32>(SnapshotLength);
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay payload materialization was cancelled.");
        return false;
    }
    Replay.commands.resize(CommandCount);
    for (uint32 CommandIndex = 0; CommandIndex < CommandCount; ++CommandIndex)
    {
        if ((CommandIndex & 0xffU) == 0U &&
            ShouldCancel && ShouldCancel())
        {
            OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay command decoding was cancelled.");
            return false;
        }
        echoes::sim::Command& Command = Replay.commands[CommandIndex];
        uint8 Player = 0;
        uint8 Type = 0;
        uint8 BuildType = 0;
        uint8 WellChoice = 0;
        uint8 Adaptation = 0;
        uint8 Research = 0;
        uint8 Queued = 0;
        int32 RawX = 0;
        int32 RawY = 0;
        if (!ReadU64(Bytes, Offset, Command.executeTick) ||
            !ReadU8(Bytes, Offset, Player) ||
            !ReadU64(Bytes, Offset, Command.sequence) ||
            !ReadU8(Bytes, Offset, Type) ||
            !ReadU32(Bytes, Offset, Command.actor) ||
            !ReadU32(Bytes, Offset, Command.target) ||
            !ReadI32(Bytes, Offset, RawX) || !ReadI32(Bytes, Offset, RawY) ||
            !ReadU8(Bytes, Offset, BuildType) ||
            !ReadU8(Bytes, Offset, WellChoice) ||
            !ReadU8(Bytes, Offset, Adaptation) ||
            !ReadU8(Bytes, Offset, Research) ||
            !ReadU8(Bytes, Offset, Queued) || Queued > 1)
        {
            OutError = TEXT("Replay command payload is truncated.");
            return false;
        }
        Command.player = Player;
        Command.type = static_cast<echoes::sim::CommandType>(Type);
        Command.position = echoes::sim::Vec2::FromRaw(RawX, RawY);
        Command.buildType = static_cast<echoes::sim::EntityType>(BuildType);
        Command.wellChoice = static_cast<echoes::sim::FutureWellChoice>(WellChoice);
        Command.warformAdaptation =
            static_cast<echoes::sim::WarformAdaptation>(Adaptation);
        Command.researchType = static_cast<echoes::sim::ResearchType>(Research);
        Command.queue = Queued != 0;
    }
    if (Offset != PayloadEnd)
    {
        OutError = TEXT("Replay envelope contains trailing payload data.");
        return false;
    }

    FEchoesReplayEnvelope Validated;
    if (!FinalizeEnvelope(
            StoredMetadata,
            Replay,
            Validated,
            OutError,
            ShouldCancel))
    {
        return false;
    }
    if (Validated.Metadata.CoverageStartTick != StoredMetadata.CoverageStartTick ||
        Validated.Metadata.DurationTicks != StoredMetadata.DurationTicks ||
        Validated.Metadata.StatisticsCoverageTicks !=
            StoredMetadata.StatisticsCoverageTicks ||
        Validated.Metadata.Outcome != StoredMetadata.Outcome ||
        Validated.Metadata.OperationResult != StoredMetadata.OperationResult ||
        Validated.Metadata.OutcomeCause != StoredMetadata.OutcomeCause ||
        Validated.Metadata.OutcomeReasonId != StoredMetadata.OutcomeReasonId ||
        Validated.Metadata.IrreversibleRecordId !=
            StoredMetadata.IrreversibleRecordId ||
        Validated.Metadata.FinalChecksum != StoredMetadata.FinalChecksum ||
        Validated.Metadata.RulesIdentity != StoredMetadata.RulesIdentity ||
        Validated.Metadata.ContentIdentity != StoredMetadata.ContentIdentity ||
        Validated.Metadata.PlayerFactions != StoredMetadata.PlayerFactions)
    {
        OutError = TEXT("Replay metadata does not match authoritative playback.");
        return false;
    }
    OutEnvelope = MoveTemp(Validated);
    return true;
}

bool FEchoesMatchReplayStore::SaveAtomic(
    const FString& Directory,
    const FEchoesReplayEnvelope& Envelope,
    FString& OutPath,
    FString& OutError)
{
    OutPath.Reset();
    FEchoesReplayEnvelope Validated;
    if (!FinalizeEnvelope(
            Envelope.Metadata, Envelope.Replay, Validated, OutError))
    {
        return false;
    }
    TArray<uint8> Bytes;
    if (!Encode(Validated, Bytes, OutError))
    {
        return false;
    }
    if (!IFileManager::Get().MakeDirectory(*Directory, true))
    {
        OutError = TEXT("Replay directory could not be created.");
        return false;
    }
    const FString Destination = FPaths::Combine(
        Directory, Validated.Metadata.ReplayId + TEXT(".echoesreplay"));
    const FString Temporary = Destination + TEXT(".tmp");
    if (IFileManager::Get().FileExists(*Destination))
    {
        OutError = TEXT("Replay ID already exists; the existing recording was preserved.");
        return false;
    }
    IFileManager::Get().Delete(*Temporary, false, true, true);
    if (!FFileHelper::SaveArrayToFile(Bytes, *Temporary))
    {
        OutError = TEXT("Replay temporary file could not be written.");
        return false;
    }
    TArray<uint8> Readback;
    FEchoesReplayEnvelope Verified;
    if (!FFileHelper::LoadFileToArray(Readback, *Temporary) ||
        !Decode(Readback, Verified, OutError))
    {
        IFileManager::Get().Delete(*Temporary, false, true, true);
        if (OutError.IsEmpty())
        {
            OutError = TEXT("Replay temporary file could not be verified.");
        }
        return false;
    }
    if (!AtomicReplace(Destination, Temporary))
    {
        IFileManager::Get().Delete(*Temporary, false, true, true);
        OutError = TEXT("Replay temporary file could not be committed.");
        return false;
    }
    OutPath = Destination;
    return true;
}

bool FEchoesMatchReplayStore::Load(
    const FString& Path,
    FEchoesReplayEnvelope& OutEnvelope,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    OutEnvelope = {};
    OutError.Reset();
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled.");
        return false;
    }
    TArray<uint8> Bytes;
    int64 FileSize = 0;
    if (!LoadBoundedReplayFile(
            Path, MaximumReplayBytes, Bytes, FileSize, OutError,
            ShouldCancel))
    {
        return false;
    }
    (void)FileSize;
    if (!Decode(Bytes, OutEnvelope, OutError, ShouldCancel))
    {
        return false;
    }
    OutEnvelope.Metadata.FilePath = Path;
    return true;
}

TArray<FEchoesReplayMetadata> FEchoesMatchReplayStore::ListMetadata(
    const FString& Directory,
    const FEchoesReplayBrowserFilter& Filter,
    TArray<FString>& OutErrors,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    OutErrors.Reset();
    IPlatformFile& PlatformFile =
        FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*Directory))
    {
        return {};
    }
    class FBoundedReplayDirectoryVisitor final
        : public IPlatformFile::FDirectoryVisitor
    {
    public:
        explicit FBoundedReplayDirectoryVisitor(
            const FEchoesReplayCancellationCheck& InShouldCancel)
            : ShouldCancel(InShouldCancel)
        {
        }

        bool Visit(const TCHAR* Path, bool bIsDirectory) override
        {
            if (ShouldCancel && ShouldCancel())
            {
                bCancelled = true;
                return false;
            }
            const FString Name = FPaths::GetCleanFilename(Path);
            ++VisitedEntries;
            NameCharacters += Name.Len();
            if (VisitedEntries > MaximumReplayBrowserEntries ||
                NameCharacters > MaximumReplayBrowserNameCharacters)
            {
                bLimitReached = true;
                return false;
            }
            if (bIsDirectory)
            {
                return true;
            }
            if (!Name.EndsWith(
                    TEXT(".echoesreplay"), ESearchCase::IgnoreCase))
            {
                return true;
            }
            if (Names.Num() >= MaximumReplayBrowserEntries)
            {
                bLimitReached = true;
                return false;
            }
            Names.Add(Name);
            return true;
        }

        const FEchoesReplayCancellationCheck& ShouldCancel;
        TArray<FString> Names;
        int32 VisitedEntries = 0;
        int64 NameCharacters = 0;
        bool bCancelled = false;
        bool bLimitReached = false;
    };

    FBoundedReplayDirectoryVisitor Visitor(ShouldCancel);
    const bool bEnumerationCompleted =
        PlatformFile.IterateDirectory(*Directory, Visitor);
    if (Visitor.bCancelled)
    {
        OutErrors.Add(TEXT("[REPLAY_BROWSER_CANCELLED] Replay browser directory enumeration was cancelled."));
        return {};
    }
    if (Visitor.bLimitReached)
    {
        OutErrors.Add(TEXT("[REPLAY_BROWSER_LIMIT] Replay browser directory entry bound was reached."));
        return {};
    }
    if (!bEnumerationCompleted)
    {
        OutErrors.Add(TEXT("[REPLAY_BROWSER_DIRECTORY_FAILED] Replay browser directory could not be enumerated."));
        return {};
    }
    TArray<FString> Names = MoveTemp(Visitor.Names);
    Names.Sort();
    TArray<FEchoesReplayMetadata> Results;
    int64 RemainingScanBytes = MaximumReplayBrowserScanBytes;
    for (const FString& Name : Names)
    {
        if (ShouldCancel && ShouldCancel())
        {
            Results.Reset();
            OutErrors.Add(TEXT("[REPLAY_BROWSER_CANCELLED] Replay browser validation was cancelled."));
            break;
        }
        const FString Path = FPaths::Combine(Directory, Name);
        TArray<uint8> Bytes;
        FString Error;
        int64 FileSize = 0;
        FEchoesReplayEnvelope Envelope;
        if (!LoadBoundedReplayFile(
                Path, RemainingScanBytes, Bytes, FileSize, Error,
                ShouldCancel))
        {
            OutErrors.Add(FString::Printf(
                TEXT("%s: %s"), *Name,
                Error.IsEmpty() ? TEXT("file could not be read") : *Error));
            if (Error.Contains(TEXT("aggregate read bound")))
            {
                Results.Reset();
                break;
            }
            if (Error.Contains(TEXT("CANCELLED")) ||
                Error.Contains(TEXT("cancelled")))
            {
                Results.Reset();
                break;
            }
            continue;
        }
        RemainingScanBytes -= FileSize;
        if (!Decode(Bytes, Envelope, Error, ShouldCancel))
        {
            OutErrors.Add(FString::Printf(
                TEXT("%s: %s"), *Name,
                Error.IsEmpty() ? TEXT("file could not be decoded") : *Error));
            if (Error.Contains(TEXT("CANCELLED")) ||
                Error.Contains(TEXT("cancelled")))
            {
                Results.Reset();
                break;
            }
            continue;
        }
        FEchoesReplayMetadata Metadata = MoveTemp(Envelope.Metadata);
        Metadata.FilePath = Path;
        if ((!Filter.MapId.IsEmpty() && Metadata.MapId != Filter.MapId) ||
            (Filter.EarliestUtc.IsSet() &&
             Metadata.RecordedUtc < Filter.EarliestUtc.GetValue()) ||
            (Filter.LatestUtc.IsSet() &&
             Metadata.RecordedUtc > Filter.LatestUtc.GetValue()))
        {
            continue;
        }
        Results.Add(MoveTemp(Metadata));
    }
    if (ShouldCancel && ShouldCancel())
    {
        Results.Reset();
        if (!OutErrors.ContainsByPredicate([](const FString& Error)
            { return Error.Contains(TEXT("CANCELLED")) ||
                     Error.Contains(TEXT("cancelled")); }))
        {
            OutErrors.Add(TEXT("[REPLAY_BROWSER_CANCELLED] Replay browser validation was cancelled."));
        }
    }
    Results.Sort([](const FEchoesReplayMetadata& Left,
                    const FEchoesReplayMetadata& Right)
    {
        if (Left.RecordedUtc != Right.RecordedUtc)
        {
            return Left.RecordedUtc > Right.RecordedUtc;
        }
        return Left.ReplayId < Right.ReplayId;
    });
    return Results;
}

bool FEchoesReplayPlaybackSession::Initialize(
    const FEchoesReplayEnvelope& Envelope,
    const FString& ExpectedBuildIdentity,
    const FString& ExpectedRulesIdentity,
    const FString& ExpectedContentIdentity,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled before playback initialization.");
        return false;
    }
    FReplayBaselineAuthority BaselineAuthority;
    if (!LoadReplayBaselineAuthority(
            Envelope.Metadata,
            Envelope.Replay,
            BaselineAuthority,
            OutError,
            ShouldCancel))
    {
        return false;
    }
    if (ExpectedBuildIdentity.IsEmpty() || ExpectedRulesIdentity.IsEmpty() ||
        ExpectedContentIdentity.IsEmpty() ||
        Envelope.Metadata.BuildIdentity != ExpectedBuildIdentity ||
        Envelope.Metadata.RulesIdentity != ExpectedRulesIdentity ||
        Envelope.Metadata.RulesIdentity != BaselineAuthority.RulesIdentity ||
        Envelope.Metadata.ContentIdentity != ExpectedContentIdentity ||
        ExpectedContentIdentity != BaselineAuthority.ContentIdentity)
    {
        OutError = TEXT("Replay build, rules, or content identity does not match this runtime.");
        return false;
    }
    return InitializeRecord(
        Envelope.Replay,
        MoveTemp(BaselineAuthority.Baseline),
        OutError,
        ShouldCancel);
}

bool FEchoesReplayPlaybackSession::InitializeRecord(
    const echoes::sim::ReplayRecord& Replay,
    std::optional<echoes::sim::Simulation> Baseline,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    std::string Error;
    if (!echoes::sim::Simulation::BuildMatchReport(
            Replay,
            &Error,
            MakeCoreCancellationCheck(ShouldCancel)).has_value())
    {
        OutError = Error == "replay validation cancelled"
            ? TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled before playback initialization.")
            : FString::Printf(
                  TEXT("Replay validation failed: %s"),
                  UTF8_TO_TCHAR(Error.c_str()));
        return false;
    }
    if (!Baseline.has_value())
    {
        OutError = TEXT("Replay baseline authority was not retained for playback initialization.");
        return false;
    }
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled before playback state was staged.");
        return false;
    }
    FEchoesReplayPlaybackSession CandidateSession;
    CandidateSession.ReplayRecord = Replay;
    std::sort(
        CandidateSession.ReplayRecord.commands.begin(),
        CandidateSession.ReplayRecord.commands.end(),
        ReplayCommandLess);
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay validation was cancelled before playback state was staged.");
        return false;
    }
    const uint64 BaselineTick = Baseline->CurrentTick();
    if (!CandidateSession.RebuildAt(
            BaselineTick,
            OutError,
            ShouldCancel,
            MoveTemp(Baseline)))
    {
        return false;
    }
    *this = MoveTemp(CandidateSession);
    return true;
}

void FEchoesReplayPlaybackSession::SetSpeed(EEchoesReplaySpeed InSpeed)
{
    Speed = InSpeed;
    HalfSpeedPhase = 0;
}

bool FEchoesReplayPlaybackSession::SetPerspective(
    EEchoesReplayPerspective InPerspective,
    FString& OutError)
{
    OutError.Reset();
    if (InPerspective == EEchoesReplayPerspective::OmniscientObserver)
    {
        Perspective = InPerspective;
        return true;
    }
    const uint8 Player = static_cast<uint8>(InPerspective);
    if (!Simulation.has_value() || Simulation->FindPlayer(Player) == nullptr)
    {
        OutError = TEXT("Replay perspective does not identify an active player.");
        return false;
    }
    Perspective = InPerspective;
    return true;
}

bool FEchoesReplayPlaybackSession::AdvanceOneCadence(FString& OutError)
{
    OutError.Reset();
    if (bPaused)
    {
        return true;
    }
    uint64 Ticks = 1;
    switch (Speed)
    {
        case EEchoesReplaySpeed::Half:
            HalfSpeedPhase ^= 1;
            Ticks = HalfSpeedPhase == 0 ? 1 : 0;
            break;
        case EEchoesReplaySpeed::Normal: Ticks = 1; break;
        case EEchoesReplaySpeed::Double: Ticks = 2; break;
        case EEchoesReplaySpeed::Quadruple: Ticks = 4; break;
        case EEchoesReplaySpeed::Octuple: Ticks = 8; break;
    }
    return AdvanceTicks(Ticks, OutError);
}

bool FEchoesReplayPlaybackSession::StepForward(FString& OutError)
{
    if (!bPaused)
    {
        OutError = TEXT("Tick step is available only while replay playback is paused.");
        return false;
    }
    return AdvanceTicks(1, OutError);
}

bool FEchoesReplayPlaybackSession::Seek(
    uint64 Tick,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel)
{
    return RebuildAt(Tick, OutError, ShouldCancel);
}

const echoes::sim::Simulation* FEchoesReplayPlaybackSession::GetSimulation() const
{
    return Simulation.has_value() ? &*Simulation : nullptr;
}

std::optional<echoes::sim::PlayerView>
FEchoesReplayPlaybackSession::GetPlayerView() const
{
    if (!Simulation.has_value() ||
        Perspective == EEchoesReplayPerspective::OmniscientObserver)
    {
        return std::nullopt;
    }
    return Simulation->CreatePlayerView(static_cast<uint8>(Perspective));
}

uint64 FEchoesReplayPlaybackSession::GetCurrentTick() const
{
    return Simulation.has_value() ? Simulation->CurrentTick() : 0;
}

bool FEchoesReplayPlaybackSession::RebuildAt(
    uint64 Tick,
    FString& OutError,
    const FEchoesReplayCancellationCheck& ShouldCancel,
    std::optional<echoes::sim::Simulation> Candidate)
{
    OutError.Reset();
    std::string Error;
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay rebuild was cancelled.");
        return false;
    }
    if (!Candidate.has_value())
    {
        Candidate = echoes::sim::Simulation::LoadSnapshot(
            ReplayRecord.initialSnapshot,
            &Error,
            echoes::sim::kDefaultHostilityMasks,
            MakeCoreCancellationCheck(ShouldCancel));
    }
    if (!Candidate.has_value())
    {
        OutError = Error == "snapshot load cancelled"
            ? TEXT("[REPLAY_VALIDATION_CANCELLED] Replay rebuild was cancelled.")
            : FString::Printf(
                  TEXT("Replay baseline failed to load: %s"),
                  UTF8_TO_TCHAR(Error.c_str()));
        return false;
    }
    const uint64 BaselineTick = Candidate->CurrentTick();
    if (Tick < BaselineTick || Tick > ReplayRecord.finalTick)
    {
        OutError = TEXT("Replay seek tick is outside the recorded interval.");
        return false;
    }
    std::size_t CommandIndex = 0;
    while (Candidate->CurrentTick() < Tick)
    {
        if (ShouldCancel && ShouldCancel())
        {
            OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay tick rebuild was cancelled.");
            return false;
        }
        const uint64 CurrentTick = Candidate->CurrentTick();
        while (CommandIndex < ReplayRecord.commands.size() &&
               ReplayRecord.commands[CommandIndex].executeTick == CurrentTick)
        {
            if ((CommandIndex & 0xffU) == 0U &&
                ShouldCancel && ShouldCancel())
            {
                OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay command rebuild was cancelled.");
                return false;
            }
            std::string Rejection;
            if (!Candidate->QueueCommand(
                    ReplayRecord.commands[CommandIndex], &Rejection))
            {
                OutError = FString::Printf(
                    TEXT("Replay command was rejected during seek: %s"),
                    UTF8_TO_TCHAR(Rejection.c_str()));
                return false;
            }
            ++CommandIndex;
        }
        if (CommandIndex < ReplayRecord.commands.size() &&
            ReplayRecord.commands[CommandIndex].executeTick < CurrentTick)
        {
            OutError = TEXT("Replay command schedule precedes the rebuilt playback tick.");
            return false;
        }
        Candidate->Step();
    }
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay tick rebuild was cancelled.");
        return false;
    }
    if (Tick == ReplayRecord.finalTick)
    {
        while (CommandIndex < ReplayRecord.commands.size())
        {
            if ((CommandIndex & 0xffU) == 0U &&
                ShouldCancel && ShouldCancel())
            {
                OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay command rebuild was cancelled.");
                return false;
            }
            std::string Rejection;
            if (!Candidate->QueueCommand(
                    ReplayRecord.commands[CommandIndex], &Rejection))
            {
                OutError = FString::Printf(
                    TEXT("Replay pending command was rejected during seek: %s"),
                    UTF8_TO_TCHAR(Rejection.c_str()));
                return false;
            }
            ++CommandIndex;
        }
        if (ReplayRecord.forfeitingPlayer != echoes::sim::kNeutralPlayer &&
            !Candidate->ForfeitPlayer(ReplayRecord.forfeitingPlayer))
        {
            OutError = TEXT("Replay concession could not be applied at its terminal tick.");
            return false;
        }
        if (Candidate->StateChecksum() != ReplayRecord.finalChecksum)
        {
            OutError = TEXT("Replay seek state does not match the recorded final checksum.");
            return false;
        }
    }
    if (ShouldCancel && ShouldCancel())
    {
        OutError = TEXT("[REPLAY_VALIDATION_CANCELLED] Replay rebuild was cancelled before publication.");
        return false;
    }
    Simulation = MoveTemp(Candidate);
    NextCommandIndex = CommandIndex;
    HalfSpeedPhase = 0;
    return true;
}

bool FEchoesReplayPlaybackSession::AdvanceTicks(
    uint64 TickCount,
    FString& OutError)
{
    OutError.Reset();
    if (!Simulation.has_value())
    {
        OutError = TEXT("Replay playback has not been initialized.");
        return false;
    }
    const uint64 BeforeTick = Simulation->CurrentTick();
    const uint64 Remaining = ReplayRecord.finalTick - BeforeTick;
    const uint64 TargetTick = BeforeTick + FMath::Min(TickCount, Remaining);
    while (Simulation->CurrentTick() < TargetTick)
    {
        const uint64 CurrentTick = Simulation->CurrentTick();
        while (NextCommandIndex < ReplayRecord.commands.size() &&
               ReplayRecord.commands[NextCommandIndex].executeTick ==
                   CurrentTick)
        {
            std::string Rejection;
            if (!Simulation->QueueCommand(
                    ReplayRecord.commands[NextCommandIndex], &Rejection))
            {
                OutError = FString::Printf(
                    TEXT("Replay command was rejected during playback: %s"),
                    UTF8_TO_TCHAR(Rejection.c_str()));
                return false;
            }
            ++NextCommandIndex;
        }
        if (NextCommandIndex < ReplayRecord.commands.size() &&
            ReplayRecord.commands[NextCommandIndex].executeTick < CurrentTick)
        {
            OutError = TEXT("Replay command schedule precedes the current playback tick.");
            return false;
        }
        Simulation->Step();
    }
    if (BeforeTick < ReplayRecord.finalTick &&
        Simulation->CurrentTick() == ReplayRecord.finalTick &&
        NextCommandIndex < ReplayRecord.commands.size())
    {
        while (NextCommandIndex < ReplayRecord.commands.size())
        {
            std::string Rejection;
            if (!Simulation->QueueCommand(
                    ReplayRecord.commands[NextCommandIndex], &Rejection))
            {
                OutError = FString::Printf(
                    TEXT("Replay pending command was rejected during playback: %s"),
                    UTF8_TO_TCHAR(Rejection.c_str()));
                return false;
            }
            ++NextCommandIndex;
        }
    }
    if (BeforeTick < ReplayRecord.finalTick &&
        Simulation->CurrentTick() == ReplayRecord.finalTick &&
        ReplayRecord.forfeitingPlayer != echoes::sim::kNeutralPlayer &&
        !Simulation->ForfeitPlayer(ReplayRecord.forfeitingPlayer))
    {
        OutError = TEXT("Replay concession could not be applied at its terminal tick.");
        return false;
    }
    if (BeforeTick < ReplayRecord.finalTick &&
        Simulation->CurrentTick() == ReplayRecord.finalTick &&
        Simulation->StateChecksum() != ReplayRecord.finalChecksum)
    {
        OutError = TEXT("Replay playback state does not match the recorded final checksum.");
        return false;
    }
    return true;
}
