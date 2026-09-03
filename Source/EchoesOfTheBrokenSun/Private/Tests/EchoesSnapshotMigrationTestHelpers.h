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

inline int32 ReadInt32(const TArray<uint8>& Bytes, int32 Offset)
{
    return static_cast<int32>(ReadUint32(Bytes, Offset));
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

// Where the variable-length regions of one embedded snapshot sit.
//
// MemoryLedger* describe the schema-25 block of per-player remembered terrain
// and remembered permanent objects. Schema 24 has no such block, and there the
// offset stays INDEX_NONE with a zero size — which is the honest answer, not a
// placeholder.
struct FEmbeddedSnapshotLayout final
{
    int32 SnapshotOffset = INDEX_NONE;
    uint32 SnapshotLength = 0;
    int32 ReceiptBlockOffset = INDEX_NONE;
    uint32 ReceiptCount = 0;
    int32 ReceiptBlockSize = 0;
    int32 MemoryLedgerOffset = INDEX_NONE;
    int32 MemoryLedgerSize = 0;
    uint32 RememberedTileCount = 0;
    uint32 RememberedObjectCount = 0;
};

inline bool SelectUniqueReceiptCandidate(
    const TArray<FEmbeddedSnapshotLayout>& LoadableCandidates,
    FEmbeddedSnapshotLayout& OutLayout)
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

// Distance from the start of a snapshot payload to the terrain grid's length
// word. Every field the writer emits ahead of that grid — magic, version, map
// dimensions, seed, protection mask, the whole rules table, per-player state —
// is fixed size, so this distance is a constant of the build.
//
// It is MEASURED from a snapshot this build actually writes rather than copied
// in as a literal, because a literal would silently rot the next time the rules
// table gains a field. The probe is a bare two-by-two simulation: no players
// joined, no entities, no pending commands, no receipts, so everything after
// the grids is a run of empty length words.
inline int32 EmbeddedSnapshotTerrainGridOffset()
{
    static const int32 Measured = []() -> int32
    {
        constexpr int32 ProbeTileCount = 2 * 2;
        constexpr int32 GridLengthSize = 4;
        // Terrain, one explored grid per player, one remembered-terrain grid
        // per player.
        constexpr int32 GridCount =
            1 + 2 * static_cast<int32>(echoes::sim::kMaximumPlayers);
        constexpr int32 EmptyTrailingBytes =
            // One empty remembered-object ledger per player, then the empty
            // entity, pending-command and receipt ledgers.
            static_cast<int32>(echoes::sim::kMaximumPlayers) * 4 + 4 + 4 + 4;
        constexpr int32 SnapshotSignatureSize = 8;
        const echoes::sim::Simulation Probe(
            echoes::sim::SimulationConfig{2, 2, 20, 0});
        const std::vector<std::uint8_t> ProbeSnapshot = Probe.SaveSnapshot();
        const int64 GridOffset = static_cast<int64>(ProbeSnapshot.size()) -
            static_cast<int64>(GridCount) *
                (GridLengthSize + ProbeTileCount) -
            EmptyTrailingBytes - SnapshotSignatureSize;
        return GridOffset > 0 && GridOffset <= MAX_int32
            ? static_cast<int32>(GridOffset)
            : INDEX_NONE;
    }();
    return Measured;
}

// Walks the fog and memory grids of one embedded snapshot and records where the
// schema-25 memory ledgers live. The walk asserts that every grid declares
// exactly the map's tile count, so a layout drift fails here rather than
// silently shifting a later field.
//
// bHasMemoryLedger is the caller's schema claim, not a guess: schemas 20 to 24
// wrote no memory at all, and for those the ledger is genuinely absent.
inline bool ResolveEmbeddedSnapshotMemoryLedger(
    const TArray<uint8>& Envelope,
    int32 SnapshotOffset,
    uint32 SnapshotLength,
    bool bHasMemoryLedger,
    FEmbeddedSnapshotLayout& InOutLayout)
{
    constexpr int32 SnapshotSignatureSize = 8;
    constexpr int32 MapWidthOffset = 8;
    constexpr int32 MapHeightOffset = 12;
    constexpr int32 GridLengthSize = 4;
    constexpr int32 SerializedRememberedObjectSize = 24;
    constexpr int32 PlayerCount =
        static_cast<int32>(echoes::sim::kMaximumPlayers);
    // Terrain plus one explored grid per player: everything schema 24 wrote.
    constexpr int32 FogGridCount = 1 + PlayerCount;

    InOutLayout.MemoryLedgerOffset = INDEX_NONE;
    InOutLayout.MemoryLedgerSize = 0;
    InOutLayout.RememberedTileCount = 0;
    InOutLayout.RememberedObjectCount = 0;

    const int32 TerrainGridOffset = EmbeddedSnapshotTerrainGridOffset();
    if (TerrainGridOffset == INDEX_NONE || SnapshotOffset < 0 ||
        SnapshotLength <= static_cast<uint32>(SnapshotSignatureSize) ||
        SnapshotLength > static_cast<uint32>(MAX_int32) ||
        static_cast<int64>(SnapshotOffset) + SnapshotLength > Envelope.Num())
    {
        return false;
    }
    const int64 PayloadEnd = static_cast<int64>(SnapshotOffset) +
        SnapshotLength - SnapshotSignatureSize;
    const int64 MapWidth =
        ReadInt32(Envelope, SnapshotOffset + MapWidthOffset);
    const int64 MapHeight =
        ReadInt32(Envelope, SnapshotOffset + MapHeightOffset);
    if (MapWidth <= 0 || MapHeight <= 0 ||
        MapWidth > static_cast<int64>(MAX_int32) / MapHeight)
    {
        return false;
    }
    const int64 TileCount = MapWidth * MapHeight;
    const int64 GridStride = GridLengthSize + TileCount;
    const int32 GridCount =
        bHasMemoryLedger ? FogGridCount + PlayerCount : FogGridCount;
    const int64 GridsBegin =
        static_cast<int64>(SnapshotOffset) + TerrainGridOffset;
    if (GridsBegin + static_cast<int64>(GridCount) * GridStride > PayloadEnd)
    {
        return false;
    }
    for (int32 GridIndex = 0; GridIndex < GridCount; ++GridIndex)
    {
        const int64 LengthOffset =
            GridsBegin + static_cast<int64>(GridIndex) * GridStride;
        if (ReadUint32(Envelope, static_cast<int32>(LengthOffset)) !=
            static_cast<uint32>(TileCount))
        {
            return false;
        }
    }
    if (!bHasMemoryLedger)
    {
        return true;
    }

    const int64 LedgerBegin =
        GridsBegin + static_cast<int64>(FogGridCount) * GridStride;
    int64 Cursor = GridsBegin + static_cast<int64>(GridCount) * GridStride;
    int64 RememberedObjects = 0;
    for (int32 Player = 0; Player < PlayerCount; ++Player)
    {
        if (Cursor + GridLengthSize > PayloadEnd)
        {
            return false;
        }
        const uint32 Count = ReadUint32(Envelope, static_cast<int32>(Cursor));
        if (Count >
            static_cast<uint32>(echoes::sim::kMaximumRememberedObjects))
        {
            return false;
        }
        Cursor += GridLengthSize +
            static_cast<int64>(Count) * SerializedRememberedObjectSize;
        if (Cursor > PayloadEnd)
        {
            return false;
        }
        RememberedObjects += Count;
    }
    InOutLayout.MemoryLedgerOffset = static_cast<int32>(LedgerBegin);
    InOutLayout.MemoryLedgerSize = static_cast<int32>(Cursor - LedgerBegin);
    InOutLayout.RememberedTileCount = static_cast<uint32>(TileCount);
    InOutLayout.RememberedObjectCount =
        static_cast<uint32>(RememberedObjects);
    return true;
}

// Locates the trailing command-resolution receipt block of one embedded
// snapshot, and on schema 25 the memory ledgers as well.
//
// The receipt block carries no self-describing terminator, so it is found by
// proposing each possible receipt count, reducing the payload to the shape the
// PREVIOUS schema wrote, and asking the real loader whether that reduction is a
// valid schema-23 snapshot. Schema 23 parses nothing after the pending
// commands and rejects trailing payload data outright, so a proposal that is
// off by even one receipt cannot load: the surplus bytes it leaves behind, or
// the command bytes it eats, are fatal. Exactly one proposal survives.
//
// Reducing a schema-25 payload means dropping the memory ledgers as well as the
// receipts. The ledgers sit in the MIDDLE of the payload, not at an end, so
// this is not a family that one blind tail-strip can serve — hence the explicit
// ledger walk above.
//
// Call sites must pass ExpectedSnapshotVersion deliberately. A checkpoint the
// test just SAVED is native (kSnapshotVersion). A checkpoint the test
// HAND-BUILT to prove migration still works is whatever old version it was
// built as, and passing the native version there would silently delete that
// backward-compatibility coverage.
inline bool InspectEmbeddedSnapshot(
    const TArray<uint8>& Envelope,
    int32 FixedHeaderSize,
    int32 LedgerLengthOffset,
    int32 SnapshotLengthOffset,
    FEmbeddedSnapshotLayout& OutLayout,
    uint32 ExpectedSnapshotVersion = echoes::sim::kSnapshotVersion)
{
    constexpr int32 SnapshotVersionOffset = 4;
    constexpr int32 ProtectionMaskOffset = 28;
    constexpr int32 ReceiptCountSize = 4;
    constexpr int32 SerializedReceiptSize = 19;
    constexpr int32 SnapshotSignatureSize = 8;
    constexpr int32 EnvelopeChecksumSize = 4;
    constexpr uint32 MemorySnapshotVersion = 25U;
    constexpr uint32 ReceiptFreeSnapshotVersion = 23U;
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
            Envelope, SnapshotOffset, SnapshotLength,
            ExpectedSnapshotVersion) ||
        Envelope[SnapshotOffset + ProtectionMaskOffset] != 0U)
    {
        return false;
    }

    FEmbeddedSnapshotLayout MeasuredLayout;
    MeasuredLayout.SnapshotOffset = SnapshotOffset;
    MeasuredLayout.SnapshotLength = SnapshotLength;
    if (!ResolveEmbeddedSnapshotMemoryLedger(
            Envelope,
            SnapshotOffset,
            SnapshotLength,
            ExpectedSnapshotVersion >= MemorySnapshotVersion,
            MeasuredLayout))
    {
        return false;
    }

    const int32 SnapshotSignatureOffset =
        SnapshotOffset + static_cast<int32>(SnapshotLength) -
        SnapshotSignatureSize;
    TArray<FEmbeddedSnapshotLayout> LoadableCandidates;
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
            ReceiptBlockOffset <
                MeasuredLayout.MemoryLedgerOffset +
                    MeasuredLayout.MemoryLedgerSize ||
            ReadUint32(Envelope, ReceiptBlockOffset) != ReceiptCount)
        {
            continue;
        }

        // The receipt block trails the memory ledgers, so removing it first
        // leaves the ledger offset undisturbed.
        TArray<uint8> Candidate = Envelope;
        Candidate.RemoveAt(
            ReceiptBlockOffset,
            ReceiptBlockSize,
            EAllowShrinking::No);
        if (MeasuredLayout.MemoryLedgerSize > 0)
        {
            Candidate.RemoveAt(
                MeasuredLayout.MemoryLedgerOffset,
                MeasuredLayout.MemoryLedgerSize,
                EAllowShrinking::No);
        }
        const uint32 V23SnapshotLength = SnapshotLength -
            static_cast<uint32>(ReceiptBlockSize) -
            static_cast<uint32>(MeasuredLayout.MemoryLedgerSize);
        WriteUint32(
            Candidate, SnapshotLengthOffset, V23SnapshotLength);
        WriteUint32(
            Candidate,
            SnapshotOffset + SnapshotVersionOffset,
            ReceiptFreeSnapshotVersion);
        if (!ResignEmbeddedSnapshot(
                Candidate, SnapshotOffset, V23SnapshotLength))
        {
            continue;
        }
        UpdateEnvelopeChecksum(Candidate);
        if (!IsLoadableEmbeddedSnapshot(
                Candidate, SnapshotOffset, V23SnapshotLength,
                ReceiptFreeSnapshotVersion))
        {
            continue;
        }
        FEmbeddedSnapshotLayout CandidateLayout = MeasuredLayout;
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

// Walks a checkpoint this build just wrote down every schema step it supports,
// stopping at the genuine schema-22 shape:
//
//   25 -> 24   drop the per-player terrain and object memory ledgers
//   24 -> 23   drop the command-resolution receipt block
//   23 -> 22   drop the protected-Command-Core mask byte
//
// Each step is proved by handing the result to the real loader and demanding it
// load at that exact version, so a wrong split cannot slip through as a smaller
// but still plausible payload. A schema-24 source simply starts one step in.
//
// The envelope is left untouched unless every step succeeds.
inline bool ConvertEmbeddedSnapshotToV22(
    TArray<uint8>& Envelope,
    int32 FixedHeaderSize,
    int32 LedgerLengthOffset,
    int32 SnapshotLengthOffset)
{
    constexpr int32 SnapshotVersionOffset = 4;
    constexpr uint32 ReceiptSnapshotVersion = 24U;
    constexpr uint32 ReceiptFreeSnapshotVersion = 23U;
    FEmbeddedSnapshotLayout Layout;
    if (!InspectEmbeddedSnapshot(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset,
            Layout))
    {
        return false;
    }

    TArray<uint8> Working = Envelope;
    uint32 WorkingSnapshotLength = Layout.SnapshotLength;
    FEmbeddedSnapshotLayout ReceiptLayout = Layout;
    if (Layout.MemoryLedgerSize > 0)
    {
        Working.RemoveAt(
            Layout.MemoryLedgerOffset,
            Layout.MemoryLedgerSize,
            EAllowShrinking::No);
        WorkingSnapshotLength -=
            static_cast<uint32>(Layout.MemoryLedgerSize);
        WriteUint32(
            Working, SnapshotLengthOffset, WorkingSnapshotLength);
        WriteUint32(
            Working,
            Layout.SnapshotOffset + SnapshotVersionOffset,
            ReceiptSnapshotVersion);
        if (!ResignEmbeddedSnapshot(
                Working, Layout.SnapshotOffset, WorkingSnapshotLength))
        {
            return false;
        }
        UpdateEnvelopeChecksum(Working);
        // The intermediate has to be a genuine schema-24 checkpoint, not just a
        // shorter one: it loads at 24, it declares no memory ledger, and the
        // receipt block rediscovered inside it is the same block the native
        // source declared.
        if (!IsLoadableEmbeddedSnapshot(
                Working, Layout.SnapshotOffset, WorkingSnapshotLength,
                ReceiptSnapshotVersion) ||
            !InspectEmbeddedSnapshot(
                Working,
                FixedHeaderSize,
                LedgerLengthOffset,
                SnapshotLengthOffset,
                ReceiptLayout,
                ReceiptSnapshotVersion) ||
            ReceiptLayout.MemoryLedgerSize != 0 ||
            ReceiptLayout.ReceiptCount != Layout.ReceiptCount ||
            ReceiptLayout.SnapshotOffset != Layout.SnapshotOffset)
        {
            return false;
        }
    }

    Working.RemoveAt(
        ReceiptLayout.ReceiptBlockOffset,
        ReceiptLayout.ReceiptBlockSize,
        EAllowShrinking::No);
    const uint32 V23SnapshotLength = WorkingSnapshotLength -
        static_cast<uint32>(ReceiptLayout.ReceiptBlockSize);
    WriteUint32(Working, SnapshotLengthOffset, V23SnapshotLength);
    WriteUint32(
        Working,
        Layout.SnapshotOffset + SnapshotVersionOffset,
        ReceiptFreeSnapshotVersion);
    if (!ResignEmbeddedSnapshot(
            Working, Layout.SnapshotOffset, V23SnapshotLength))
    {
        return false;
    }
    UpdateEnvelopeChecksum(Working);
    if (!IsLoadableEmbeddedSnapshot(
            Working, Layout.SnapshotOffset, V23SnapshotLength,
            ReceiptFreeSnapshotVersion) ||
        !ConvertEmbeddedSnapshotV23ToV22(
            Working,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset))
    {
        return false;
    }
    const uint64 ExpectedShrink = 5ULL +
        static_cast<uint64>(Layout.ReceiptCount) * 19ULL +
        static_cast<uint64>(Layout.MemoryLedgerSize);
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

inline bool InspectMission14EnvelopeSnapshot(
    const TArray<uint8>& Envelope,
    FEmbeddedSnapshotLayout& OutLayout,
    uint32 ExpectedSnapshotVersion = echoes::sim::kSnapshotVersion)
{
    constexpr int32 FixedHeaderSize = 19;
    constexpr int32 LedgerLengthOffset = 11;
    constexpr int32 SnapshotLengthOffset = 15;
    return HasMission14EnvelopeHeader(Envelope) &&
        InspectEmbeddedSnapshot(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset,
            OutLayout,
            ExpectedSnapshotVersion);
}

inline bool InspectMission15EnvelopeSnapshot(
    const TArray<uint8>& Envelope,
    FEmbeddedSnapshotLayout& OutLayout,
    uint32 ExpectedSnapshotVersion = echoes::sim::kSnapshotVersion)
{
    constexpr int32 FixedHeaderSize = 38;
    constexpr int32 LedgerLengthOffset = 30;
    constexpr int32 SnapshotLengthOffset = 34;
    return HasMission15EnvelopeHeader(Envelope) &&
        InspectEmbeddedSnapshot(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset,
            OutLayout,
            ExpectedSnapshotVersion);
}

inline bool ConvertMission14EnvelopeSnapshotToV22(TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 19;
    constexpr int32 LedgerLengthOffset = 11;
    constexpr int32 SnapshotLengthOffset = 15;
    return HasMission14EnvelopeHeader(Envelope) &&
        ConvertEmbeddedSnapshotToV22(
            Envelope,
            FixedHeaderSize,
            LedgerLengthOffset,
            SnapshotLengthOffset);
}

inline bool ConvertMission15EnvelopeSnapshotToV22(TArray<uint8>& Envelope)
{
    constexpr int32 FixedHeaderSize = 38;
    constexpr int32 LedgerLengthOffset = 30;
    constexpr int32 SnapshotLengthOffset = 34;
    return HasMission15EnvelopeHeader(Envelope) &&
        ConvertEmbeddedSnapshotToV22(
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
