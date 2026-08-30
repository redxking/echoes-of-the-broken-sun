#pragma once

#include "CoreMinimal.h"

/** Shared responsive field-HUD geometry and battlefield-visibility checks. */
struct FEchoesHudLayout final
{
    FBox2D MainPanel;
    FBox2D ObjectivePanel;
    FBox2D CommandDeckPanel;
    FBox2D StatusPanel;
    FBox2D MinimapPanel;
    bool bObjectiveVisible = false;
    bool bCommandDeckVisible = false;
    bool bStatusVisible = false;
    bool bMinimapVisible = false;

    [[nodiscard]] static FEchoesHudLayout Build(
        const FVector2D& ViewportSize,
        float HudScale,
        bool bHasStatusMessage)
    {
        FEchoesHudLayout Layout;
        const float Width = FMath::Max(1.0f, ViewportSize.X);
        const float Height = FMath::Max(1.0f, ViewportSize.Y);
        const float Scale = FMath::Clamp(HudScale, 0.85f, 1.35f);

        const float MainWidth = FMath::Min(
            920.0f * Scale,
            FMath::Max(320.0f, Width - 36.0f));
        Layout.MainPanel = FBox2D(
            FVector2D(18.0f, 18.0f),
            FVector2D(18.0f + MainWidth, 18.0f + 276.0f * Scale));

        const float ObjectiveWidth =
            FMath::Clamp(460.0f * Scale, 390.0f, 560.0f);
        const float ObjectiveHeight =
            FMath::Clamp(178.0f * Scale, 160.0f, 212.0f);
        float ObjectiveLeft = Width - ObjectiveWidth - 20.0f;
        float ObjectiveTop = 18.0f;
        if (ObjectiveLeft < Layout.MainPanel.Max.X + 20.0f)
        {
            ObjectiveLeft = 18.0f;
            ObjectiveTop = Layout.MainPanel.Max.Y + 16.0f;
        }
        Layout.ObjectivePanel = FBox2D(
            FVector2D(ObjectiveLeft, ObjectiveTop),
            FVector2D(
                ObjectiveLeft + ObjectiveWidth,
                ObjectiveTop + ObjectiveHeight));
        Layout.bObjectiveVisible =
            ObjectiveLeft >= 18.0f &&
            Layout.ObjectivePanel.Max.X <= Width - 18.0f &&
            Layout.ObjectivePanel.Max.Y <= Height - 24.0f;

        const float CommandWidth = FMath::Min(
            900.0f * Scale,
            FMath::Max(520.0f, Width - 300.0f));
        const float CommandHeight = 112.0f * Scale;
        const float CommandTop = Height - 84.0f - CommandHeight;
        Layout.CommandDeckPanel = FBox2D(
            FVector2D(18.0f, CommandTop),
            FVector2D(18.0f + CommandWidth, CommandTop + CommandHeight));
        Layout.bCommandDeckVisible =
            CommandTop >= Layout.MainPanel.Max.Y + 12.0f;

        const float StatusWidth = FMath::Min(920.0f * Scale, Width - 36.0f);
        Layout.StatusPanel = FBox2D(
            FVector2D(18.0f, Height - 72.0f),
            FVector2D(18.0f + StatusWidth, Height - 24.0f));
        Layout.bStatusVisible =
            bHasStatusMessage && StatusWidth > 0.0f && Height >= 96.0f;

        const float MinimapSize = FMath::Clamp(
            FMath::Min(220.0f * Scale, Height * 0.30f),
            150.0f,
            240.0f);
        const float MinimapLeft = Width - MinimapSize - 20.0f;
        const float MinimapTop = Height - MinimapSize - 92.0f;
        Layout.MinimapPanel = FBox2D(
            FVector2D(MinimapLeft, MinimapTop),
            FVector2D(
                MinimapLeft + MinimapSize,
                MinimapTop + MinimapSize));
        Layout.bMinimapVisible =
            MinimapLeft >= 18.0f &&
            MinimapTop >= Layout.MainPanel.Max.Y + 16.0f;
        return Layout;
    }

    [[nodiscard]] bool IsBattlefieldPointClear(
        const FVector2D& ScreenPosition,
        const FVector2D& ViewportSize,
        float Margin = 12.0f) const
    {
        if (ScreenPosition.X < Margin || ScreenPosition.Y < Margin ||
            ScreenPosition.X > ViewportSize.X - Margin ||
            ScreenPosition.Y > ViewportSize.Y - Margin)
        {
            return false;
        }
        const auto IsInsideExpanded = [ScreenPosition, Margin](
                                          const FBox2D& Panel)
        {
            return ScreenPosition.X >= Panel.Min.X - Margin &&
                   ScreenPosition.X <= Panel.Max.X + Margin &&
                   ScreenPosition.Y >= Panel.Min.Y - Margin &&
                   ScreenPosition.Y <= Panel.Max.Y + Margin;
        };
        return !IsInsideExpanded(MainPanel) &&
               (!bObjectiveVisible || !IsInsideExpanded(ObjectivePanel)) &&
               (!bCommandDeckVisible || !IsInsideExpanded(CommandDeckPanel)) &&
               (!bStatusVisible || !IsInsideExpanded(StatusPanel)) &&
               (!bMinimapVisible || !IsInsideExpanded(MinimapPanel));
    }
};
