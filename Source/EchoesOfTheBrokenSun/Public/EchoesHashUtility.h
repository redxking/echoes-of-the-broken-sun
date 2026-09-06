#pragma once

#include "CoreMinimal.h"

namespace EchoesHash
{
/** SHA-256 of a byte array as lowercase hex. Matches the Python compilers. */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API FString ComputeSha256Hex(
    const TArray<uint8>& Bytes);

/**
 * Value-only SHA-256 path for large immutable buffers. Cancellation is checked
 * between bounded input chunks; an empty result with OutCancelled=true never
 * represents a digest.
 */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API FString ComputeSha256Hex(
    TConstArrayView<uint8> Bytes,
    const TFunction<bool()>& ShouldCancel,
    bool& OutCancelled);
}
