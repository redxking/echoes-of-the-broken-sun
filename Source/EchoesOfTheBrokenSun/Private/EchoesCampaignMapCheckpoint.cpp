#include "EchoesCampaignMapCheckpoint.h"

#include "Containers/StringConv.h"
#include "Misc/Crc.h"

namespace
{
constexpr uint8 EnvelopeMagic[] = {'E', 'C', 'M', 'A', 'P', 'C', 'P', '1'};
constexpr int32 Sha256HexLength = 64;
constexpr int32 MaximumIdentifierBytes = 256;
constexpr int32 FixedHeaderBytes = UE_ARRAY_COUNT(EnvelopeMagic) + 2 + 1 +
    Sha256HexLength + Sha256HexLength + 4;
constexpr int32 ChecksumBytes = 4;

void AppendCheckpointU16(TArray<uint8>& Bytes, uint16 Value)
{
    Bytes.Add(static_cast<uint8>(Value));
    Bytes.Add(static_cast<uint8>(Value >> 8));
}

void AppendCheckpointU32(TArray<uint8>& Bytes, uint32 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

bool ReadCheckpointU16(const TArray<uint8>& Bytes, int32& Offset, uint16& OutValue)
{
    if (Offset < 0 || Bytes.Num() - Offset < 2)
    {
        return false;
    }
    OutValue = static_cast<uint16>(Bytes[Offset]) |
        (static_cast<uint16>(Bytes[Offset + 1]) << 8);
    Offset += 2;
    return true;
}

bool ReadCheckpointU32(const TArray<uint8>& Bytes, int32& Offset, uint32& OutValue)
{
    if (Offset < 0 || Bytes.Num() - Offset < 4)
    {
        return false;
    }
    OutValue = 0;
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        OutValue |= static_cast<uint32>(Bytes[Offset + ByteIndex]) <<
            (ByteIndex * 8);
    }
    Offset += 4;
    return true;
}

bool IsCanonicalSha256(const FString& Value)
{
    if (Value.Len() != Sha256HexLength)
    {
        return false;
    }
    for (const TCHAR Character : Value)
    {
        if (!((Character >= '0' && Character <= '9') ||
              (Character >= 'a' && Character <= 'f')))
        {
            return false;
        }
    }
    return true;
}

bool AppendIdentifier(TArray<uint8>& Bytes, const FString& Identifier)
{
    FTCHARToUTF8 Utf8(*Identifier);
    const int32 Length = Utf8.Length();
    if (Length <= 0 || Length > MaximumIdentifierBytes || Length > MAX_uint16)
    {
        return false;
    }
    AppendCheckpointU16(Bytes, static_cast<uint16>(Length));
    Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Length);
    return true;
}

bool ReadIdentifier(const TArray<uint8>& Bytes, int32& Offset, FString& OutIdentifier)
{
    uint16 ByteLength = 0;
    if (!ReadCheckpointU16(Bytes, Offset, ByteLength) || ByteLength == 0 ||
        ByteLength > MaximumIdentifierBytes || Bytes.Num() - Offset < ByteLength)
    {
        return false;
    }

    const ANSICHAR* Data = reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + Offset);
    FUTF8ToTCHAR Converted(Data, ByteLength);
    FString Candidate(Converted.Length(), Converted.Get());
    FTCHARToUTF8 RoundTrip(*Candidate);
    if (RoundTrip.Length() != ByteLength ||
        FMemory::Memcmp(RoundTrip.Get(), Data, ByteLength) != 0)
    {
        return false;
    }
    Offset += ByteLength;
    OutIdentifier = Candidate;
    return true;
}

bool AppendSha256(TArray<uint8>& Bytes, const FString& Value)
{
    if (!IsCanonicalSha256(Value))
    {
        return false;
    }
    FTCHARToUTF8 Utf8(*Value);
    check(Utf8.Length() == Sha256HexLength);
    Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Sha256HexLength);
    return true;
}

bool ReadSha256(const TArray<uint8>& Bytes, int32& Offset, FString& OutValue)
{
    if (Offset < 0 || Bytes.Num() - Offset < Sha256HexLength)
    {
        return false;
    }
    const ANSICHAR* Data = reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + Offset);
    FUTF8ToTCHAR Converted(Data, Sha256HexLength);
    FString Candidate(Converted.Length(), Converted.Get());
    Offset += Sha256HexLength;
    if (!IsCanonicalSha256(Candidate))
    {
        return false;
    }
    OutValue = MoveTemp(Candidate);
    return true;
}
}

bool FEchoesCampaignMapCheckpointIdentity::IsBound() const
{
    return MissionOrdinal >= 1 && MissionOrdinal <= 15 && !Doctrine.IsEmpty() &&
        !MapId.IsEmpty() && IsCanonicalSha256(SourceSha256) &&
        IsCanonicalSha256(TerrainIdentitySha256);
}

bool FEchoesCampaignMapCheckpoint::Wrap(
    const FEchoesCampaignMapCheckpointIdentity& Identity,
    const TArray<uint8>& Payload,
    TArray<uint8>& OutEnvelope,
    EEchoesCampaignMapCheckpointFailure& OutFailure)
{
    OutFailure = EEchoesCampaignMapCheckpointFailure::None;
    if (!Identity.IsBound())
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Unbound;
        return false;
    }
    if (Payload.Num() <= 0 || Payload.Num() > MaximumPayloadBytes)
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Integrity;
        return false;
    }

    TArray<uint8> Candidate;
    Candidate.Reserve(FixedHeaderBytes + Identity.Doctrine.Len() + Identity.MapId.Len() +
        Payload.Num() + ChecksumBytes);
    Candidate.Append(EnvelopeMagic, UE_ARRAY_COUNT(EnvelopeMagic));
    AppendCheckpointU16(Candidate, Version);
    Candidate.Add(Identity.MissionOrdinal);
    if (!AppendIdentifier(Candidate, Identity.Doctrine) ||
        !AppendIdentifier(Candidate, Identity.MapId) ||
        !AppendSha256(Candidate, Identity.SourceSha256) ||
        !AppendSha256(Candidate, Identity.TerrainIdentitySha256))
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Unbound;
        return false;
    }
    AppendCheckpointU32(Candidate, static_cast<uint32>(Payload.Num()));
    Candidate.Append(Payload);
    AppendCheckpointU32(Candidate, FCrc::MemCrc32(Candidate.GetData(), Candidate.Num()));

    OutEnvelope = MoveTemp(Candidate);
    return true;
}

bool FEchoesCampaignMapCheckpoint::Extract(
    const TArray<uint8>& Envelope,
    const FEchoesCampaignMapCheckpointIdentity& ExpectedIdentity,
    TArray<uint8>& OutPayload,
    EEchoesCampaignMapCheckpointFailure& OutFailure)
{
    OutFailure = EEchoesCampaignMapCheckpointFailure::None;
    if (!ExpectedIdentity.IsBound())
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Unbound;
        return false;
    }
    FEchoesCampaignMapCheckpointIdentity RecordedIdentity;
    TArray<uint8> CandidatePayload;
    if (!Inspect(Envelope, RecordedIdentity, CandidatePayload, OutFailure))
    {
        return false;
    }
    if (RecordedIdentity != ExpectedIdentity)
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Stale;
        return false;
    }

    OutPayload = MoveTemp(CandidatePayload);
    return true;
}

bool FEchoesCampaignMapCheckpoint::Inspect(
    const TArray<uint8>& Envelope,
    FEchoesCampaignMapCheckpointIdentity& OutIdentity,
    TArray<uint8>& OutPayload,
    EEchoesCampaignMapCheckpointFailure& OutFailure)
{
    OutFailure = EEchoesCampaignMapCheckpointFailure::None;
    if (Envelope.Num() < UE_ARRAY_COUNT(EnvelopeMagic) ||
        FMemory::Memcmp(Envelope.GetData(), EnvelopeMagic, UE_ARRAY_COUNT(EnvelopeMagic)) != 0)
    {
        // A legacy campaign snapshot has no map-binding envelope and is therefore unbound.
        OutFailure = EEchoesCampaignMapCheckpointFailure::Unbound;
        return false;
    }
    if (Envelope.Num() < FixedHeaderBytes + ChecksumBytes)
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Integrity;
        return false;
    }

    const int32 ChecksumOffset = Envelope.Num() - ChecksumBytes;
    int32 ChecksumReadOffset = ChecksumOffset;
    uint32 StoredChecksum = 0;
    if (!ReadCheckpointU32(Envelope, ChecksumReadOffset, StoredChecksum) ||
        StoredChecksum != FCrc::MemCrc32(Envelope.GetData(), ChecksumOffset))
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Integrity;
        return false;
    }

    int32 Offset = UE_ARRAY_COUNT(EnvelopeMagic);
    uint16 EnvelopeVersion = 0;
    if (!ReadCheckpointU16(Envelope, Offset, EnvelopeVersion))
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Integrity;
        return false;
    }
    if (EnvelopeVersion != Version)
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Unsupported;
        return false;
    }
    if (!Envelope.IsValidIndex(Offset))
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Integrity;
        return false;
    }

    FEchoesCampaignMapCheckpointIdentity RecordedIdentity;
    RecordedIdentity.MissionOrdinal = Envelope[Offset++];
    if (!ReadIdentifier(Envelope, Offset, RecordedIdentity.Doctrine) ||
        !ReadIdentifier(Envelope, Offset, RecordedIdentity.MapId) ||
        !ReadSha256(Envelope, Offset, RecordedIdentity.SourceSha256) ||
        !ReadSha256(Envelope, Offset, RecordedIdentity.TerrainIdentitySha256))
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Integrity;
        return false;
    }

    uint32 PayloadLength = 0;
    if (!ReadCheckpointU32(Envelope, Offset, PayloadLength) ||
        PayloadLength == 0 || PayloadLength > static_cast<uint32>(MaximumPayloadBytes) ||
        PayloadLength > static_cast<uint32>(MAX_int32) ||
        Offset > ChecksumOffset ||
        static_cast<int64>(Offset) + static_cast<int64>(PayloadLength) != ChecksumOffset)
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Integrity;
        return false;
    }
    if (!RecordedIdentity.IsBound())
    {
        OutFailure = EEchoesCampaignMapCheckpointFailure::Unbound;
        return false;
    }

    TArray<uint8> Candidate;
    Candidate.Append(Envelope.GetData() + Offset, static_cast<int32>(PayloadLength));
    OutIdentity = MoveTemp(RecordedIdentity);
    OutPayload = MoveTemp(Candidate);
    return true;
}
