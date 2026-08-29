#pragma once

#include "CoreMinimal.h"

/** Screen-safe placement for one anonymous world-space vibration cue. */
struct FEchoesContactIndicatorPlacement final
{
    FVector2D MarkerPosition = FVector2D::ZeroVector;
    FVector2D LabelPosition = FVector2D::ZeroVector;
    bool bClampedToEdge = false;
    bool bLabelOnLeft = false;
};

/** Pure layout model shared by the HUD and presentation automation. */
struct FEchoesContactIndicatorLayout final
{
    [[nodiscard]] static FEchoesContactIndicatorPlacement Calculate(
        const FVector2D& ProjectedPosition,
        const FVector2D& ViewportSize,
        float HudScale);

    [[nodiscard]] static FString BuildPrimaryLabel(
        int32 Ordinal,
        bool bClampedToEdge);
};
