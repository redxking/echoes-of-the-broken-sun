#include "EchoesHashUtility.h"

namespace EchoesHash
{
namespace
{
constexpr uint32 Sha256RoundConstants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

[[nodiscard]] constexpr uint32 RotateRight(const uint32 Value, const uint32 Count)
{
    return (Value >> Count) | (Value << (32U - Count));
}

void TransformSha256Block(const uint8* Block, uint32 (&State)[8])
{
    uint32 Schedule[64]{};
    for (int32 Index = 0; Index < 16; ++Index)
    {
        const int32 Offset = Index * 4;
        Schedule[Index] =
            (static_cast<uint32>(Block[Offset]) << 24U) |
            (static_cast<uint32>(Block[Offset + 1]) << 16U) |
            (static_cast<uint32>(Block[Offset + 2]) << 8U) |
            static_cast<uint32>(Block[Offset + 3]);
    }
    for (int32 Index = 16; Index < 64; ++Index)
    {
        const uint32 S0 =
            RotateRight(Schedule[Index - 15], 7U) ^
            RotateRight(Schedule[Index - 15], 18U) ^
            (Schedule[Index - 15] >> 3U);
        const uint32 S1 =
            RotateRight(Schedule[Index - 2], 17U) ^
            RotateRight(Schedule[Index - 2], 19U) ^
            (Schedule[Index - 2] >> 10U);
        Schedule[Index] = Schedule[Index - 16] + S0 +
            Schedule[Index - 7] + S1;
    }

    uint32 A = State[0];
    uint32 B = State[1];
    uint32 C = State[2];
    uint32 D = State[3];
    uint32 E = State[4];
    uint32 F = State[5];
    uint32 G = State[6];
    uint32 H = State[7];
    for (int32 Index = 0; Index < 64; ++Index)
    {
        const uint32 Sum1 = RotateRight(E, 6U) ^ RotateRight(E, 11U) ^
            RotateRight(E, 25U);
        const uint32 Choice = (E & F) ^ ((~E) & G);
        const uint32 Temporary1 = H + Sum1 + Choice +
            Sha256RoundConstants[Index] + Schedule[Index];
        const uint32 Sum0 = RotateRight(A, 2U) ^ RotateRight(A, 13U) ^
            RotateRight(A, 22U);
        const uint32 Majority = (A & B) ^ (A & C) ^ (B & C);
        const uint32 Temporary2 = Sum0 + Majority;

        H = G;
        G = F;
        F = E;
        E = D + Temporary1;
        D = C;
        C = B;
        B = A;
        A = Temporary1 + Temporary2;
    }

    State[0] += A;
    State[1] += B;
    State[2] += C;
    State[3] += D;
    State[4] += E;
    State[5] += F;
    State[6] += G;
    State[7] += H;
}
}

[[nodiscard]] FString ComputeSha256Hex(
    TConstArrayView<uint8> Bytes,
    const TFunction<bool()>& ShouldCancel,
    bool& OutCancelled)
{
    OutCancelled = false;
    uint32 State[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };

    constexpr int32 CancellationChunkBytes = 1024 * 1024;
    int32 Offset = 0;
    int32 BytesUntilCancellationCheck = 0;
    while (Bytes.Num() - Offset >= 64)
    {
        if (BytesUntilCancellationCheck <= 0)
        {
            if (ShouldCancel && ShouldCancel())
            {
                OutCancelled = true;
                return {};
            }
            BytesUntilCancellationCheck = CancellationChunkBytes;
        }
        TransformSha256Block(Bytes.GetData() + Offset, State);
        Offset += 64;
        BytesUntilCancellationCheck -= 64;
    }

    if (ShouldCancel && ShouldCancel())
    {
        OutCancelled = true;
        return {};
    }

    uint8 FinalBlocks[128]{};
    const int32 Remaining = Bytes.Num() - Offset;
    if (Remaining > 0)
    {
        FMemory::Memcpy(FinalBlocks, Bytes.GetData() + Offset, Remaining);
    }
    FinalBlocks[Remaining] = 0x80U;
    const int32 FinalLength = Remaining < 56 ? 64 : 128;
    const uint64 BitLength = static_cast<uint64>(Bytes.Num()) * 8ULL;
    for (int32 Index = 0; Index < 8; ++Index)
    {
        FinalBlocks[FinalLength - 1 - Index] =
            static_cast<uint8>(BitLength >> (Index * 8));
    }
    TransformSha256Block(FinalBlocks, State);
    if (FinalLength == 128)
    {
        TransformSha256Block(FinalBlocks + 64, State);
    }

    FString Digest;
    Digest.Reserve(64);
    for (const uint32 Word : State)
    {
        Digest += FString::Printf(TEXT("%08x"), Word);
    }
    return Digest;
}

[[nodiscard]] FString ComputeSha256Hex(const TArray<uint8>& Bytes)
{
    bool bCancelled = false;
    return ComputeSha256Hex(
        MakeArrayView(Bytes.GetData(), Bytes.Num()), {}, bCancelled);
}
}
