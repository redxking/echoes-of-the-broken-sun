#pragma once

#include "CoreMinimal.h"

namespace EchoesHash
{
/** SHA-256 of a byte array as lowercase hex. Matches the Python compilers. */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API FString ComputeSha256Hex(
    const TArray<uint8>& Bytes);
}
