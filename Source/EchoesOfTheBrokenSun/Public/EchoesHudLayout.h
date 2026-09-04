#pragma once

#include "CoreMinimal.h"

/** Shared responsive field-HUD geometry and battlefield-visibility checks. */
struct FEchoesHudLayout final
{
    // Field-HUD arrangement (A8, 2026-09-04): the RTS convention players
    // already know - a full-width bottom bar carrying the minimap at the left,
    // the selection panel in the middle, and the command card at the right;
    // resources top-right; the command strip and objectives top-left; the
    // status line just above the bar. Every panel is drawn in the Compact
    // command-deck language of the Bible, not borrowed art.
    FBox2D MainPanel;        // top-left command strip: title, selection, research
    FBox2D ObjectivePanel;   // top-left, under the command strip
    FBox2D ResourcePanel;    // top-right ledger readout
    FBox2D BottomBar;        // full-width instrument bar
    FBox2D MinimapPanel;     // inside the bar, left
    FBox2D SelectionPanel;   // inside the bar, centre
    FBox2D CommandDeckPanel; // inside the bar, right (command card)
    FBox2D StatusPanel;      // status message, above the bar, left
    bool bObjectiveVisible = false;
    bool bResourceVisible = false;
    bool bBottomBarVisible = false;
    bool bMinimapVisible = false;
    bool bSelectionVisible = false;
    bool bCommandDeckVisible = false;
    bool bStatusVisible = false;

    [[nodiscard]] static FEchoesHudLayout Build(
        const FVector2D& ViewportSize,
        float HudScale,
        bool bHasStatusMessage)
    {
        FEchoesHudLayout Layout;
        const float Width = FMath::Max(1.0f, ViewportSize.X);
        const float Height = FMath::Max(1.0f, ViewportSize.Y);
        const float Scale = FMath::Clamp(HudScale, 0.85f, 1.35f);
        constexpr float Edge = 18.0f;
        constexpr float Gap = 14.0f;

        // Bottom bar
        float BarHeight = FMath::Clamp(212.0f * Scale, 176.0f, 262.0f);
        if (Height < 540.0f)
        {
            BarHeight = FMath::Clamp(Height * 0.30f, 140.0f, 262.0f);
        }
        Layout.BottomBar = FBox2D(
            FVector2D(0.0f, Height - BarHeight), FVector2D(Width, Height));
        Layout.bBottomBarVisible = Height >= 360.0f;
        const float InnerTop = Layout.BottomBar.Min.Y + Gap;
        const float InnerBottom = Height - Gap;

        // Minimap: a square at the bar's left
        const float MinimapSize = InnerBottom - InnerTop;
        Layout.MinimapPanel = FBox2D(
            FVector2D(Gap, InnerTop),
            FVector2D(Gap + MinimapSize, InnerBottom));
        Layout.bMinimapVisible = Layout.bBottomBarVisible && MinimapSize >= 120.0f;

        // Command card: the bar's right
        const float DeckWidth = FMath::Clamp(468.0f * Scale, 400.0f, 560.0f);
        Layout.CommandDeckPanel = FBox2D(
            FVector2D(Width - Gap - DeckWidth, InnerTop),
            FVector2D(Width - Gap, InnerBottom));
        const float LeftOfDeck = Layout.bMinimapVisible ? Layout.MinimapPanel.Max.X + Gap : Gap;
        Layout.bCommandDeckVisible =
            Layout.bBottomBarVisible &&
            Layout.CommandDeckPanel.Min.X >= LeftOfDeck &&
            InnerBottom - InnerTop >= 120.0f;

        // Selection: whatever the bar has left between the two
        const float SelectionRight =
            Layout.bCommandDeckVisible ? Layout.CommandDeckPanel.Min.X - Gap : Width - Gap;
        Layout.SelectionPanel = FBox2D(
            FVector2D(LeftOfDeck, InnerTop), FVector2D(SelectionRight, InnerBottom));
        Layout.bSelectionVisible =
            Layout.bBottomBarVisible && Layout.SelectionPanel.GetSize().X >= 220.0f;

        // Top-left command strip
        const float MainWidth = FMath::Clamp(560.0f * Scale, 320.0f, FMath::Max(320.0f, Width * 0.36f));
        Layout.MainPanel = FBox2D(
            FVector2D(Edge, Edge),
            FVector2D(Edge + MainWidth, Edge + 96.0f * Scale));

        // Top-right resource ledger
        const float ResourceWidth = FMath::Clamp(720.0f * Scale, 620.0f, 860.0f);
        Layout.ResourcePanel = FBox2D(
            FVector2D(Width - Edge - ResourceWidth, Edge),
            FVector2D(Width - Edge, Edge + 60.0f * Scale));
        Layout.bResourceVisible =
            Layout.ResourcePanel.Min.X >= Layout.MainPanel.Max.X + Gap;

        // Objectives under the command strip
        const float ObjectiveWidth = FMath::Clamp(440.0f * Scale, 380.0f, 520.0f);
        const float ObjectiveHeight = FMath::Clamp(150.0f * Scale, 132.0f, 190.0f);
        const float ObjectiveTop = Layout.MainPanel.Max.Y + 16.0f;
        Layout.ObjectivePanel = FBox2D(
            FVector2D(Edge, ObjectiveTop),
            FVector2D(Edge + ObjectiveWidth, ObjectiveTop + ObjectiveHeight));

        // Status line just above the bar
        const float StatusHeight = 44.0f;
        const float StatusTop = Layout.BottomBar.Min.Y - 12.0f - StatusHeight;
        const float StatusWidth = FMath::Min(760.0f * Scale, Width - 2.0f * Edge);
        Layout.StatusPanel = FBox2D(
            FVector2D(Edge, StatusTop),
            FVector2D(Edge + StatusWidth, StatusTop + StatusHeight));
        Layout.bStatusVisible =
            bHasStatusMessage && StatusWidth > 0.0f && StatusTop > Layout.MainPanel.Max.Y + 8.0f;
        Layout.bObjectiveVisible =
            Layout.ObjectivePanel.Max.X <= Width - Edge &&
            Layout.ObjectivePanel.Max.Y + 8.0f <=
                (Layout.bStatusVisible ? Layout.StatusPanel.Min.Y : Layout.BottomBar.Min.Y - 12.0f);
        return Layout;
    }

    /** True when a pointer position lies on any drawn chrome panel. */
    [[nodiscard]] bool IsPointerOnChrome(const FVector2D& ScreenPosition) const
    {
        return MainPanel.IsInsideOrOn(ScreenPosition) ||
               (bResourceVisible && ResourcePanel.IsInsideOrOn(ScreenPosition)) ||
               (bBottomBarVisible && BottomBar.IsInsideOrOn(ScreenPosition)) ||
               (bCommandDeckVisible && CommandDeckPanel.IsInsideOrOn(ScreenPosition)) ||
               (bObjectiveVisible && ObjectivePanel.IsInsideOrOn(ScreenPosition)) ||
               (bStatusVisible && StatusPanel.IsInsideOrOn(ScreenPosition)) ||
               (bMinimapVisible && MinimapPanel.IsInsideOrOn(ScreenPosition));
    }

    [[nodiscard]] bool IsBattlefieldPointClear(
        const FVector2D& ScreenPosition,
        const FVector2D& ViewportSize,
        float Margin = 12.0f) const
    {
        return IsBattlefieldBoxClear(
            FBox2D(ScreenPosition, ScreenPosition),
            ViewportSize,
            Margin);
    }

    [[nodiscard]] bool IsBattlefieldBoxClear(
        const FBox2D& ScreenBounds,
        const FVector2D& ViewportSize,
        float Margin = 12.0f) const
    {
        if (ScreenBounds.Min.X < Margin || ScreenBounds.Min.Y < Margin ||
            ScreenBounds.Max.X > ViewportSize.X - Margin ||
            ScreenBounds.Max.Y > ViewportSize.Y - Margin)
        {
            return false;
        }
        const auto OverlapsExpanded = [ScreenBounds, Margin](
                                          const FBox2D& Panel)
        {
            return ScreenBounds.Max.X >= Panel.Min.X - Margin &&
                   ScreenBounds.Min.X <= Panel.Max.X + Margin &&
                   ScreenBounds.Max.Y >= Panel.Min.Y - Margin &&
                   ScreenBounds.Min.Y <= Panel.Max.Y + Margin;
        };
        return !OverlapsExpanded(MainPanel) &&
               (!bResourceVisible || !OverlapsExpanded(ResourcePanel)) &&
               (!bBottomBarVisible || !OverlapsExpanded(BottomBar)) &&
               (!bObjectiveVisible || !OverlapsExpanded(ObjectivePanel)) &&
               (!bCommandDeckVisible || !OverlapsExpanded(CommandDeckPanel)) &&
               (!bStatusVisible || !OverlapsExpanded(StatusPanel)) &&
               (!bMinimapVisible || !OverlapsExpanded(MinimapPanel));
    }
};
