#include "EchoesContactIndicatorLayout.h"

FEchoesContactIndicatorPlacement FEchoesContactIndicatorLayout::Calculate(
    const FVector2D& ProjectedPosition,
    const FVector2D& ViewportSize,
    float HudScale)
{
    FEchoesContactIndicatorPlacement Placement;
    if (ViewportSize.X <= 1.0f || ViewportSize.Y <= 1.0f)
    {
        return Placement;
    }

    const float Scale = FMath::Clamp(HudScale, 0.75f, 1.50f);
    const float MarkerMargin = 30.0f * Scale;
    const float SafeMinX = MarkerMargin;
    const float SafeMaxX = FMath::Max(SafeMinX, ViewportSize.X - MarkerMargin);
    const float SafeMinY = FMath::Min(300.0f, ViewportSize.Y * 0.42f);
    const float BottomReserve =
        FMath::Min(228.0f * Scale, ViewportSize.Y * 0.34f);
    const float SafeMaxY =
        FMath::Max(SafeMinY, ViewportSize.Y - BottomReserve);

    Placement.MarkerPosition = FVector2D(
        FMath::Clamp(ProjectedPosition.X, SafeMinX, SafeMaxX),
        FMath::Clamp(ProjectedPosition.Y, SafeMinY, SafeMaxY));
    Placement.bClampedToEdge =
        !Placement.MarkerPosition.Equals(ProjectedPosition, 0.5f);
    Placement.bLabelOnLeft =
        Placement.MarkerPosition.X > ViewportSize.X * 0.5f;

    const float LabelWidth = 270.0f * Scale;
    const float LabelGap = 22.0f * Scale;
    const float DesiredLabelX = Placement.bLabelOnLeft
        ? Placement.MarkerPosition.X - LabelGap - LabelWidth
        : Placement.MarkerPosition.X + LabelGap;
    Placement.LabelPosition.X = FMath::Clamp(
        DesiredLabelX,
        18.0f * Scale,
        FMath::Max(18.0f * Scale, ViewportSize.X - LabelWidth - 18.0f * Scale));
    Placement.LabelPosition.Y = FMath::Clamp(
        Placement.MarkerPosition.Y - 15.0f * Scale,
        SafeMinY,
        FMath::Max(SafeMinY, ViewportSize.Y - 48.0f * Scale));
    return Placement;
}

FString FEchoesContactIndicatorLayout::BuildPrimaryLabel(
    int32 Ordinal,
    bool bClampedToEdge)
{
    const int32 SafeOrdinal = FMath::Max(1, Ordinal);
    if (bClampedToEdge)
    {
        return FString::Printf(
            TEXT("ANONYMOUS VIBRATION %02d  //  EDGE"), SafeOrdinal);
    }
    return FString::Printf(
        TEXT("ANONYMOUS VIBRATION %02d"), SafeOrdinal);
}
