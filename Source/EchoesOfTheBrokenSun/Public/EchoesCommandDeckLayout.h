#pragma once

#include "CoreMinimal.h"

/**
 * Per-button geometry for the tactical command deck, laid inside the panel
 * FEchoesHudLayout::Build already positions. Draw and hit-test share these
 * boxes. Below a usable button width the deck reports no buttons and stays
 * display-only; clicks over the panel are still consumed by the caller.
 */
struct FEchoesCommandDeckLayout final
{
    static constexpr int32 MaxButtons = 6;
    static constexpr float MinimumButtonWidth = 64.0f;

    FBox2D Panel = FBox2D(ForceInit);
    TArray<FBox2D, TInlineAllocator<MaxButtons>> Buttons;

    [[nodiscard]] static FEchoesCommandDeckLayout Build(
        const FBox2D& CommandDeckPanel,
        float HudScale,
        int32 ButtonCount)
    {
        FEchoesCommandDeckLayout Layout;
        Layout.Panel = CommandDeckPanel;
        const int32 ClampedCount = FMath::Clamp(ButtonCount, 0, MaxButtons);
        if (ClampedCount == 0)
        {
            return Layout;
        }
        const float Scale = FMath::Clamp(HudScale, 0.8f, 1.5f);
        const float Margin = 16.0f * Scale;
        const float Gap = 8.0f * Scale;
        // Command card: a grid, three columns when the panel is card-shaped,
        // one row across when it is wide. Room stays below for the context line.
        const float PanelWidth = CommandDeckPanel.GetSize().X;
        const int32 Columns = FMath::Clamp(PanelWidth >= 640.0f * Scale ? ClampedCount : 3, 1, ClampedCount);
        const int32 Rows = (ClampedCount + Columns - 1) / Columns;
        const float RowTop = CommandDeckPanel.Min.Y + 56.0f * Scale;
        const float RowLeft = CommandDeckPanel.Min.X + Margin;
        const float RowRight = CommandDeckPanel.Max.X - Margin;
        const float ContextReserve = 30.0f * Scale;
        const float AvailableHeight =
            CommandDeckPanel.Max.Y - ContextReserve - RowTop - Gap * static_cast<float>(Rows - 1);
        const float ButtonHeight = FMath::Min(46.0f * Scale, AvailableHeight / static_cast<float>(Rows));
        const float ButtonWidth =
            (RowRight - RowLeft - Gap * static_cast<float>(Columns - 1)) /
            static_cast<float>(Columns);
        if (ButtonWidth < MinimumButtonWidth || ButtonHeight < 22.0f)
        {
            return Layout;
        }
        for (int32 Index = 0; Index < ClampedCount; ++Index)
        {
            const int32 Column = Index % Columns;
            const int32 Row = Index / Columns;
            const float ButtonLeft =
                RowLeft + static_cast<float>(Column) * (ButtonWidth + Gap);
            const float ButtonTop = RowTop + static_cast<float>(Row) * (ButtonHeight + Gap);
            Layout.Buttons.Add(FBox2D(
                FVector2D(ButtonLeft, ButtonTop),
                FVector2D(ButtonLeft + ButtonWidth, ButtonTop + ButtonHeight)));
        }
        return Layout;
    }

    [[nodiscard]] int32 HitTest(const FVector2D& ScreenPosition) const
    {
        for (int32 Index = 0; Index < Buttons.Num(); ++Index)
        {
            if (Buttons[Index].IsInsideOrOn(ScreenPosition))
            {
                return Index;
            }
        }
        return INDEX_NONE;
    }
};
