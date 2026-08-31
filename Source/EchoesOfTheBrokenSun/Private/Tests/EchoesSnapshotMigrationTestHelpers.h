#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
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
    if (Envelope.Num() < FixedHeaderSize + EnvelopeChecksumSize)
    {
        return false;
    }
    const uint32 LedgerLength = ReadUint32(Envelope, LedgerLengthOffset);
    const uint32 SnapshotLength = ReadUint32(Envelope, SnapshotLengthOffset);
    if (LedgerLength == 0 || SnapshotLength <= ProtectionMaskOffset +
            SnapshotSignatureSize ||
        LedgerLength > static_cast<uint32>(MAX_int32) ||
        SnapshotLength > static_cast<uint32>(MAX_int32))
    {
        return false;
    }
    const int64 SnapshotOffset64 =
        static_cast<int64>(FixedHeaderSize) + LedgerLength;
    const int64 ExpectedEnvelopeSize = SnapshotOffset64 + SnapshotLength +
        EnvelopeChecksumSize;
    if (SnapshotOffset64 > MAX_int32 ||
        ExpectedEnvelopeSize != Envelope.Num())
    {
        return false;
    }
    const int32 SnapshotOffset = static_cast<int32>(SnapshotOffset64);
    if (ReadUint32(Envelope, SnapshotOffset + SnapshotVersionOffset) != 23U)
    {
        return false;
    }

    Envelope.RemoveAt(SnapshotOffset + ProtectionMaskOffset, 1, EAllowShrinking::No);
    const uint32 PriorSnapshotLength = SnapshotLength - 1U;
    WriteUint32(Envelope, SnapshotLengthOffset, PriorSnapshotLength);
    WriteUint32(Envelope, SnapshotOffset + SnapshotVersionOffset, 22U);
    const int32 SnapshotSignatureOffset =
        SnapshotOffset + static_cast<int32>(PriorSnapshotLength) -
        SnapshotSignatureSize;
    WriteUint64(
        Envelope,
        SnapshotSignatureOffset,
        SnapshotIntegrity(
            Envelope,
            SnapshotOffset,
            static_cast<int32>(PriorSnapshotLength) - SnapshotSignatureSize));
    WriteUint32(
        Envelope,
        Envelope.Num() - EnvelopeChecksumSize,
        FCrc::MemCrc32(
            Envelope.GetData(),
            Envelope.Num() - EnvelopeChecksumSize));
    return true;
}

inline bool ConvertMission14EnvelopeSnapshotV23ToV22(TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 19;
    constexpr int32 LedgerLengthOffset = 11;
    constexpr int32 SnapshotLengthOffset = 15;
    return ConvertEmbeddedSnapshotV23ToV22(
        Envelope,
        FixedHeaderSize,
        LedgerLengthOffset,
        SnapshotLengthOffset);
}

inline bool ConvertMission15EnvelopeSnapshotV23ToV22(TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 38;
    constexpr int32 LedgerLengthOffset = 30;
    constexpr int32 SnapshotLengthOffset = 34;
    return ConvertEmbeddedSnapshotV23ToV22(
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
