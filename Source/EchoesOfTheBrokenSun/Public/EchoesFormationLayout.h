#pragma once

#include "CoreMinimal.h"

/** Player-selected destination layout; resulting commands remain authoritative. */
enum class EEchoesFormationType : uint8
{
    Box,
    Line,
    Wedge
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFormationLayout final
{
    [[nodiscard]] static TArray<FVector> BuildDestinations(
        const FVector& Anchor,
        const FVector& Forward,
        int32 UnitCount,
        EEchoesFormationType Formation,
        float SpacingWorldUnits);

    [[nodiscard]] static FString DisplayName(EEchoesFormationType Formation);
};
