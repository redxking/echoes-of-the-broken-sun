#pragma once

#include "CoreMinimal.h"

/** Shared screen-space geometry for rendering and pointer activation. */
struct FEchoesTechnologyPanelLayout final
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    FBox2D CloseButton;
    FBox2D TechnologyRows[2];

    [[nodiscard]] static FEchoesTechnologyPanelLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesTechnologyPanelLayout Layout;
        const float Scale = FMath::Clamp(HudScale, 0.75f, 1.35f);
        Layout.Size.X = FMath::Min(
            820.0f * Scale,
            FMath::Max(320.0f, ViewportSize.X - 40.0f));
        Layout.Size.Y = FMath::Min(
            540.0f * Scale,
            FMath::Max(360.0f, ViewportSize.Y - 40.0f));
        Layout.Origin = FVector2D(
            FMath::Max(20.0f, (ViewportSize.X - Layout.Size.X) * 0.5f),
            FMath::Max(20.0f, (ViewportSize.Y - Layout.Size.Y) * 0.5f));

        const float Margin = 34.0f * Scale;
        const float HeaderHeight = 138.0f * Scale;
        const float FooterHeight = 62.0f * Scale;
        const float RowGap = 16.0f * Scale;
        const float RowHeight = FMath::Max(
            82.0f * Scale,
            (Layout.Size.Y - HeaderHeight - FooterHeight - RowGap) * 0.5f);
        const float RowWidth = Layout.Size.X - Margin * 2.0f;
        const float RowX = Layout.Origin.X + Margin;
        const float FirstRowY = Layout.Origin.Y + HeaderHeight;
        Layout.TechnologyRows[0] = FBox2D(
            FVector2D(RowX, FirstRowY),
            FVector2D(RowX + RowWidth, FirstRowY + RowHeight));
        Layout.TechnologyRows[1] = FBox2D(
            FVector2D(RowX, FirstRowY + RowHeight + RowGap),
            FVector2D(RowX + RowWidth, FirstRowY + RowHeight * 2.0f + RowGap));

        const FVector2D CloseSize(58.0f * Scale, 34.0f * Scale);
        const FVector2D CloseMin(
            Layout.Origin.X + Layout.Size.X - Margin - CloseSize.X,
            Layout.Origin.Y + 24.0f * Scale);
        Layout.CloseButton = FBox2D(CloseMin, CloseMin + CloseSize);
        return Layout;
    }
};
