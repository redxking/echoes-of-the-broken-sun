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
        const float Scale = FMath::Clamp(HudScale, 0.85f, 1.35f);
        const float Margin = 16.0f * Scale;
        const float ButtonHeight = FMath::Min(
            28.0f * Scale,
            FMath::Max(0.0f, CommandDeckPanel.GetSize().Y * 0.34f));
        // The button row replaces the primary-action text line, above the
        // formation/context line the deck keeps drawing.
        const float RowTop = CommandDeckPanel.Min.Y + 56.0f * Scale;
        const float RowLeft = CommandDeckPanel.Min.X + Margin;
        const float RowRight = CommandDeckPanel.Max.X - Margin;
        const float Gap = 8.0f * Scale;
        const float ButtonWidth =
            (RowRight - RowLeft - Gap * static_cast<float>(ClampedCount - 1)) /
            static_cast<float>(ClampedCount);
        if (ButtonWidth < MinimumButtonWidth ||
            ButtonHeight < 12.0f ||
            RowTop + ButtonHeight > CommandDeckPanel.Max.Y)
        {
            return Layout;
        }
        for (int32 Index = 0; Index < ClampedCount; ++Index)
        {
            const float ButtonLeft =
                RowLeft + static_cast<float>(Index) * (ButtonWidth + Gap);
            Layout.Buttons.Add(FBox2D(
                FVector2D(ButtonLeft, RowTop),
                FVector2D(ButtonLeft + ButtonWidth, RowTop + ButtonHeight)));
        }
        return Layout;
    }

    /** Index of the button under a screen position, or INDEX_NONE. */
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
