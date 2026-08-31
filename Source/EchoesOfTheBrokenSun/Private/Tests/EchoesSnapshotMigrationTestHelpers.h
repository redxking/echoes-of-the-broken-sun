#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "EchoesPrologueMissionModel.h"
#include "EchoesSimCore/Simulation.h"
#include "Misc/Crc.h"

namespace EchoesSnapshotMigrationTestHelpers
{
inline uint32 ReadUint32(const TArray<uint8>& Bytes, int32 Offset)
{
    if (Offset < 0 || Bytes.Num() - Offset < 4)
    {
        return 0;
    }
    return static_cast<uint32>(Bytes[Offset]) |
        (static_cast<uint32>(Bytes[Offset + 1]) << 8U) |
        (static_cast<uint32>(Bytes[Offset + 2]) << 16U) |
        (static_cast<uint32>(Bytes[Offset + 3]) << 24U);
}

inline void WriteUint32(TArray<uint8>& Bytes, int32 Offset, uint32 Value)
{
    check(Offset >= 0 && Bytes.Num() - Offset >= 4);
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        Bytes[Offset + ByteIndex] =
            static_cast<uint8>(Value >> (ByteIndex * 8));
    }
}

inline void WriteUint64(TArray<uint8>& Bytes, int32 Offset, uint64 Value)
{
    check(Offset >= 0 && Bytes.Num() - Offset >= 8);
    for (int32 ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
    {
        Bytes[Offset + ByteIndex] =
            static_cast<uint8>(Value >> (ByteIndex * 8));
    }
}

inline uint64 SnapshotIntegrity(
    const TArray<uint8>& Bytes,
    int32 SnapshotOffset,
    int32 PayloadLength)
{
    uint64 Hash = 14695981039346656037ULL;
    for (int32 Index = 0; Index < PayloadLength; ++Index)
    {
        Hash ^= Bytes[SnapshotOffset + Index];
        Hash *= 1099511628211ULL;
    }
    return Hash;
}

struct FEmbeddedSnapshotV24Layout final
{
    int32 SnapshotOffset = INDEX_NONE;
    uint32 SnapshotLength = 0;
    int32 ReceiptBlockOffset = INDEX_NONE;
    uint32 ReceiptCount = 0;
    int32 ReceiptBlockSize = 0;
};

inline bool SelectUniqueReceiptCandidate(
    const TArray<FEmbeddedSnapshotV24Layout>& LoadableCandidates,
    FEmbeddedSnapshotV24Layout& OutLayout)
{
    OutLayout = {};
    if (LoadableCandidates.Num() != 1)
    {
        return false;
    }
    OutLayout = LoadableCandidates[0];
    return true;
}

inline bool HasValidEnvelopeChecksum(const TArray<uint8>& Envelope)
{
    constexpr int32 EnvelopeChecksumSize = 4;
    return Envelope.Num() >= EnvelopeChecksumSize &&
        ReadUint32(Envelope, Envelope.Num() - EnvelopeChecksumSize) ==
            FCrc::MemCrc32(
                Envelope.GetData(),
                Envelope.Num() - EnvelopeChecksumSize);
}

inline void UpdateEnvelopeChecksum(TArray<uint8>& Envelope)
{
    constexpr int32 EnvelopeChecksumSize = 4;
    check(Envelope.Num() >= EnvelopeChecksumSize);
    WriteUint32(
        Envelope,
        Envelope.Num() - EnvelopeChecksumSize,
        FCrc::MemCrc32(
            Envelope.GetData(),
            Envelope.Num() - EnvelopeChecksumSize));
}

inline bool ResignEmbeddedSnapshot(
    TArray<uint8>& Envelope,
    int32 SnapshotOffset,
    uint32 SnapshotLength)
{
    constexpr int32 SnapshotSignatureSize = 8;
    if (SnapshotOffset < 0 ||
        SnapshotLength <= static_cast<uint32>(SnapshotSignatureSize) ||
        SnapshotLength > static_cast<uint32>(MAX_int32) ||
        static_cast<int64>(SnapshotOffset) + SnapshotLength >
            Envelope.Num())
    {
        return false;
    }
    const int32 SnapshotLengthInt = static_cast<int32>(SnapshotLength);
    WriteUint64(
        Envelope,
        SnapshotOffset + SnapshotLengthInt - SnapshotSignatureSize,
        SnapshotIntegrity(
            Envelope,
            SnapshotOffset,
            SnapshotLengthInt - SnapshotSignatureSize));
    return true;
}

inline bool IsLoadableEmbeddedSnapshot(
    const TArray<uint8>& Envelope,
    int32 SnapshotOffset,
    uint32 SnapshotLength,
    uint32 ExpectedVersion)
{
    if (SnapshotOffset < 0 || SnapshotLength < 8U ||
        SnapshotLength > static_cast<uint32>(MAX_int32) ||
        static_cast<int64>(SnapshotOffset) + SnapshotLength >
            Envelope.Num() ||
        ReadUint32(Envelope, SnapshotOffset + 4) != ExpectedVersion)
    {
        return false;
    }
    std::string Error;
    return echoes::sim::Simulation::LoadSnapshot(
               std::span<const std::uint8_t>(
                   Envelope.GetData() + SnapshotOffset,
                   static_cast<size_t>(SnapshotLength)),
               &Error)
               .has_value() &&
        Error.empty();
}

inline bool InspectEmbeddedSnapshotV24(
    const TArray<uint8>& Envelope,
    int32 FixedHeaderSize,
    int32 LedgerLengthOffset,
    int32 SnapshotLengthOffset,
    FEmbeddedSnapshotV24Layout& OutLayout)
{
    constexpr int32 SnapshotVersionOffset = 4;
    constexpr int32 ProtectionMaskOffset = 28;
    constexpr int32 ReceiptCountSize = 4;
    constexpr int32 SerializedReceiptSize = 19;
    constexpr int32 SnapshotSignatureSize = 8;
    constexpr int32 EnvelopeChecksumSize = 4;
    OutLayout = {};
    if (FixedHeaderSize < 0 || LedgerLengthOffset < 0 ||
        SnapshotLengthOffset < 0 ||
        Envelope.Num() < FixedHeaderSize + EnvelopeChecksumSize ||
        !HasValidEnvelopeChecksum(Envelope))
    {
        return false;
    }
    const uint32 LedgerLength = ReadUint32(Envelope, LedgerLengthOffset);
    const uint32 SnapshotLength = ReadUint32(Envelope, SnapshotLengthOffset);
    if (LedgerLength == 0 ||
        SnapshotLength <= static_cast<uint32>(
            ProtectionMaskOffset + ReceiptCountSize +
            SnapshotSignatureSize) ||
        LedgerLength > static_cast<uint32>(MAX_int32) ||
        SnapshotLength > static_cast<uint32>(MAX_int32))
    {
        return false;
    }
    const int64 SnapshotOffset64 =
        static_cast<int64>(FixedHeaderSize) + LedgerLength;
    const int64 ExpectedEnvelopeSize = SnapshotOffset64 + SnapshotLength +
        EnvelopeChecksumSize;
    if (SnapshotOffset64 > MAX_int32 || SnapshotOffset64 < FixedHeaderSize ||
        ExpectedEnvelopeSize != Envelope.Num())
    {
        return false;
    }
    const int32 SnapshotOffset = static_cast<int32>(SnapshotOffset64);
    if (!IsLoadableEmbeddedSnapshot(
            Envelope, SnapshotOffset, SnapshotLength, 24U) ||
        Envelope[SnapshotOffset + ProtectionMaskOffset] != 0U)
    {
        return false;
    }

    const int32 SnapshotSignatureOffset =
        SnapshotOffset + static_cast<int32>(SnapshotLength) -
        SnapshotSignatureSize;
    TArray<FEmbeddedSnapshotV24Layout> LoadableCandidates;
    for (uint32 ReceiptCount = 0;
         ReceiptCount <= static_cast<uint32>(
             echoes::sim::kMaximumCommandResolutionReceipts);
         ++ReceiptCount)
    {
        const uint64 ReceiptBlockSize64 =
            static_cast<uint64>(ReceiptCountSize) +
            static_cast<uint64>(ReceiptCount) * SerializedReceiptSize;
        if (ReceiptBlockSize64 > static_cast<uint64>(MAX_int32) ||
            ReceiptBlockSize64 >=
                static_cast<uint64>(SnapshotLength) -
                    ProtectionMaskOffset - SnapshotSignatureSize)
        {
            break;
        }
        const int32 ReceiptBlockSize =
            static_cast<int32>(ReceiptBlockSize64);
        const int32 ReceiptBlockOffset =
            SnapshotSignatureOffset - ReceiptBlockSize;
        if (ReceiptBlockOffset <=
                SnapshotOffset + ProtectionMaskOffset ||
            ReadUint32(Envelope, ReceiptBlockOffset) != ReceiptCount)
        {
            continue;
        }

        TArray<uint8> Candidate = Envelope;
        Candidate.RemoveAt(
            ReceiptBlockOffset,
            ReceiptBlockSize,
            EAllowShrinking::No);
        const uint32 V23SnapshotLength =
            SnapshotLength - static_cast<uint32>(ReceiptBlockSize);
        WriteUint32(
            Candidate, SnapshotLengthOffset, V23SnapshotLength);
        WriteUint32(
            Candidate,
            SnapshotOffset + SnapshotVersionOffset,
            23U);
        if (!ResignEmbeddedSnapshot(
                Candidate, SnapshotOffset, V23SnapshotLength))
        {
            continue;
        }
        UpdateEnvelopeChecksum(Candidate);
        if (!IsLoadableEmbeddedSnapshot(
                Candidate, SnapshotOffset, V23SnapshotLength, 23U))
        {
            continue;
        }
        FEmbeddedSnapshotV24Layout CandidateLayout;
        CandidateLayout.SnapshotOffset = SnapshotOffset;
        CandidateLayout.SnapshotLength = SnapshotLength;
        CandidateLayout.ReceiptBlockOffset = ReceiptBlockOffset;
        CandidateLayout.ReceiptCount = ReceiptCount;
        CandidateLayout.ReceiptBlockSize = ReceiptBlockSize;
        LoadableCandidates.Add(CandidateLayout);
    }
    return SelectUniqueReceiptCandidate(LoadableCandidates, OutLayout);
}

inline bool ConvertEmbeddedSnapshotV23ToV22(
    TArray<uint8>& Envelope,
    int32 FixedHeaderSize,
    int32 LedgerLengthOffset,
    int32 SnapshotLengthOffset)
{
    constexpr int32 SnapshotVersionOffset = 4;
    constexpr int32 ProtectionMaskOffset = 28;
    constexpr int32 SnapshotSignatureSize = 8;
    constexpr int32 EnvelopeChecksumSize = 4;
    if (FixedHeaderSize < 0 || LedgerLengthOffset < 0 ||
        SnapshotLengthOffset < 0 ||
        Envelope.Num() < FixedHeaderSize + EnvelopeChecksumSize ||
        !HasValidEnvelopeChecksum(Envelope))
    {
        return false;
    }
    const uint32 LedgerLength = ReadUint32(Envelope, LedgerLengthOffset);
    const uint32 V23SnapshotLength =
        ReadUint32(Envelope, SnapshotLengthOffset);
    if (LedgerLength == 0 ||
        V23SnapshotLength <= static_cast<uint32>(
            ProtectionMaskOffset + SnapshotSignatureSize) ||
        LedgerLength > static_cast<uint32>(MAX_int32) ||
        V23SnapshotLength > static_cast<uint32>(MAX_int32))
    {
        return false;
    }
    const int64 SnapshotOffset64 =
        static_cast<int64>(FixedHeaderSize) + LedgerLength;
    if (SnapshotOffset64 > MAX_int32 || SnapshotOffset64 < FixedHeaderSize ||
        SnapshotOffset64 + V23SnapshotLength + EnvelopeChecksumSize !=
            Envelope.Num())
    {
        return false;
    }
    const int32 SnapshotOffset = static_cast<int32>(SnapshotOffset64);
    if (!IsLoadableEmbeddedSnapshot(
            Envelope, SnapshotOffset, V23SnapshotLength, 23U) ||
        Envelope[SnapshotOffset + ProtectionMaskOffset] != 0U)
    {
        return false;
    }
    TArray<uint8> Working = Envelope;
    Working.RemoveAt(
        SnapshotOffset + ProtectionMaskOffset,
        1,
        EAllowShrinking::No);
    const uint32 V22SnapshotLength = V23SnapshotLength - 1U;
    WriteUint32(Working, SnapshotLengthOffset, V22SnapshotLength);
    WriteUint32(
        Working,
        SnapshotOffset + SnapshotVersionOffset,
        22U);
    if (!ResignEmbeddedSnapshot(
            Working, SnapshotOffset, V22SnapshotLength))
    {
        return false;
    }
    UpdateEnvelopeChecksum(Working);
    if (!IsLoadableEmbeddedSnapshot(
            Working, SnapshotOffset, V22SnapshotLength, 22U))
    {
        return false;
    }
    Envelope = MoveTemp(Working);
    return true;
}

inline bool ConvertEmbeddedSnapshotV24ToV22(
    TArray<uint8>& Envelope,
    int32 FixedHeaderSize,
    int32 LedgerLengthOffset,
    int32 SnapshotLengthOffset)
{
    constexpr int32 SnapshotVersionOffset = 4;
    FEmbeddedSnapshotV24Layout Layout;
    if (!InspectEmbeddedSnapshotV24(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset,
            Layout))
    {
        return false;
    }

    TArray<uint8> Working = Envelope;
    Working.RemoveAt(
        Layout.ReceiptBlockOffset,
        Layout.ReceiptBlockSize,
        EAllowShrinking::No);
    const uint32 V23SnapshotLength =
        Layout.SnapshotLength - static_cast<uint32>(Layout.ReceiptBlockSize);
    WriteUint32(Working, SnapshotLengthOffset, V23SnapshotLength);
    WriteUint32(
        Working,
        Layout.SnapshotOffset + SnapshotVersionOffset,
        23U);
    if (!ResignEmbeddedSnapshot(
            Working, Layout.SnapshotOffset, V23SnapshotLength))
    {
        return false;
    }
    UpdateEnvelopeChecksum(Working);
    if (!IsLoadableEmbeddedSnapshot(
            Working, Layout.SnapshotOffset, V23SnapshotLength, 23U) ||
        !ConvertEmbeddedSnapshotV23ToV22(
            Working,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset))
    {
        return false;
    }
    const uint64 ExpectedShrink = 5ULL +
        static_cast<uint64>(Layout.ReceiptCount) * 19ULL;
    if (static_cast<uint64>(Envelope.Num() - Working.Num()) !=
        ExpectedShrink)
    {
        return false;
    }
    Envelope = MoveTemp(Working);
    return true;
}

inline bool HasMission14EnvelopeHeader(const TArray<uint8>& Envelope)
{
    static constexpr uint8 Magic[] = {
        'E', 'C', 'H', 'O', 'M', '1', '4', 'Q'};
    return Envelope.Num() >= 10 &&
        FMemory::Memcmp(
            Envelope.GetData(), Magic, UE_ARRAY_COUNT(Magic)) == 0 &&
        Envelope[8] == 2U &&
        Envelope[9] == static_cast<uint8>(
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand);
}

inline bool HasMission15EnvelopeHeader(const TArray<uint8>& Envelope)
{
    static constexpr uint8 Magic[] = {
        'E', 'C', 'H', 'O', 'M', '1', '5', 'Q'};
    return Envelope.Num() >= 10 &&
        FMemory::Memcmp(
            Envelope.GetData(), Magic, UE_ARRAY_COUNT(Magic)) == 0 &&
        Envelope[8] == 2U &&
        Envelope[9] == static_cast<uint8>(
            EEchoesOperationMode::CampaignTheBrokenSun);
}

inline bool InspectMission14EnvelopeSnapshotV24(
    const TArray<uint8>& Envelope,
    FEmbeddedSnapshotV24Layout& OutLayout)
{
    constexpr int32 FixedHeaderSize = 19;
    constexpr int32 LedgerLengthOffset = 11;
    constexpr int32 SnapshotLengthOffset = 15;
    return HasMission14EnvelopeHeader(Envelope) &&
        InspectEmbeddedSnapshotV24(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset,
            OutLayout);
}

inline bool InspectMission15EnvelopeSnapshotV24(
    const TArray<uint8>& Envelope,
    FEmbeddedSnapshotV24Layout& OutLayout)
{
    constexpr int32 FixedHeaderSize = 38;
    constexpr int32 LedgerLengthOffset = 30;
    constexpr int32 SnapshotLengthOffset = 34;
    return HasMission15EnvelopeHeader(Envelope) &&
        InspectEmbeddedSnapshotV24(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset,
            OutLayout);
}

inline bool ConvertMission14EnvelopeSnapshotV24ToV22(TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 19;
    constexpr int32 LedgerLengthOffset = 11;
    constexpr int32 SnapshotLengthOffset = 15;
    return HasMission14EnvelopeHeader(Envelope) &&
        ConvertEmbeddedSnapshotV24ToV22(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset);
}

inline bool ConvertMission15EnvelopeSnapshotV24ToV22(TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 38;
    constexpr int32 LedgerLengthOffset = 30;
    constexpr int32 SnapshotLengthOffset = 34;
    return HasMission15EnvelopeHeader(Envelope) &&
        ConvertEmbeddedSnapshotV24ToV22(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset);
}

inline uint32 Mission14SnapshotVersion(const TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 19;
    const uint32 LedgerLength = ReadUint32(Envelope, 11);
    if (LedgerLength > static_cast<uint32>(MAX_int32) ||
        static_cast<int64>(FixedHeaderSize) + LedgerLength + 8 >
            Envelope.Num())
    {
        return 0;
    }
    return ReadUint32(
        Envelope,
        FixedHeaderSize + static_cast<int32>(LedgerLength) + 4);
}

inline uint32 Mission15SnapshotVersion(const TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 38;
    const uint32 LedgerLength = ReadUint32(Envelope, 30);
    if (LedgerLength > static_cast<uint32>(MAX_int32) ||
        static_cast<int64>(FixedHeaderSize) + LedgerLength + 8 >
            Envelope.Num())
    {
        return 0;
    }
    return ReadUint32(
        Envelope,
        FixedHeaderSize + static_cast<int32>(LedgerLength) + 4);
}
}

#endif
