#pragma once

#include "CoreMinimal.h"

/** Shared responsive field-HUD geometry and battlefield-visibility checks. */
struct FEchoesHudLayout final
{
    // One pixel-space contract for rendered widgets and battlefield input.
    // MainPanel remains a compatibility alias for the bottom selection panel.
    FBox2D MainPanel;        // compatibility alias for SelectionPanel
    FBox2D ObjectivePanel;   // bottom console, above selection
    FBox2D ResourcePanel;    // compact global resource indicators
    FBox2D MenuPanel;        // persistent battlefield menu affordance
    FBox2D BottomBar;        // full-width instrument bar
    FBox2D MinimapPanel;     // inside the bar, left
    FBox2D SelectionPanel;   // inside the bar, centre
    FBox2D CommandDeckPanel; // inside the bar, right (command card)
    FBox2D StatusPanel;      // status message, above the bar, left
    bool bObjectiveVisible = false;
    bool bResourceVisible = false;
    bool bMenuVisible = false;
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
        const float Scale = FMath::Clamp(HudScale, 0.8f, 1.5f);
        constexpr float Edge = 18.0f;
        constexpr float Gap = 14.0f;

        // Keep the console below the battlefield. Accessibility scaling grows
        // its height, but never consumes more than the lower half of the view.
        // The console must still fit what it carries. The selection card needs
        // 96 units of text at scale, the objective header takes 90, and three
        // gaps of 14 sit between and around them. Shrinking below that sum left
        // the centre panel too short for a single selected unit at 150% on a
        // 720-line display. The lower-half ceiling still wins over the floor.
        const float SelectionTextHeight = 96.0f * Scale;
        const float HeaderTextHeight = 90.0f * Scale;
        const float MinBarHeight = SelectionTextHeight + HeaderTextHeight + 3.0f * Gap;
        const float BarHeight = FMath::Clamp(
            FMath::Min(252.0f * Scale, Height * 0.38f),
            FMath::Min(MinBarHeight, Height * 0.5f),
            Height * 0.5f);
        const float Top = Height - BarHeight;
        const float InnerTop = Top + Gap;
        const float InnerBottom = Height - Gap;
        const float HeaderHeight = 90.0f * Scale;
        const float MapSize = FMath::Max(1.0f, BarHeight - 2 * Gap);
        const float LeftWidth = FMath::Max(220.0f, MapSize);
        const float DeckWidth = FMath::Min(360.0f * Scale, Width * 0.32f);
        const float CenterLeft = Gap + LeftWidth + Gap;
        const float CenterRight = Width - 2 * Gap - DeckWidth;
        Layout.BottomBar = FBox2D(FVector2D(0, Top), FVector2D(Width, Height));
        // The concept's upper-left utility control is a real input target.
        // Reserve it before sizing telemetry so narrow/high-scale layouts never
        // stack visible controls on the same pointer bounds.
        const float MenuWidth = 108.0f * Scale;
        const float MenuHeight = 36.0f * Scale;
        Layout.MenuPanel = FBox2D(FVector2D(Edge, Edge),
            FVector2D(Edge + MenuWidth, Edge + MenuHeight));
        const float ResourceLeftLimit = Layout.MenuPanel.Max.X + Gap;
        // SPEC-UI-007 permits compact global indicators. Keep resource counts
        // readable without stacking a large ledger over the minimap.
        const float ResourceRight = FMath::Max(ResourceLeftLimit,
            Width - Gap - FMath::Max(220.0f, Width * 0.17f));
        const float ResourceWidth = FMath::Min(FMath::Min(540.0f * Scale, Width * 0.58f),
            ResourceRight - ResourceLeftLimit);
        // Leave the existing upper-right tutorial skip control unobstructed.
        // Three readable source-backed lines require more than the original
        // 64-dip proposal: one resource row plus faction and match/research
        // context. This stays clear of the upper-right tutorial affordance.
        Layout.ResourcePanel = FBox2D(FVector2D(ResourceRight - ResourceWidth, Gap),
            FVector2D(ResourceRight, Gap + 84.0f * Scale));
        Layout.MinimapPanel = FBox2D(FVector2D(Gap, InnerBottom - MapSize),
            FVector2D(Gap + MapSize, InnerBottom));
        Layout.ObjectivePanel = FBox2D(FVector2D(CenterLeft, InnerTop),
            FVector2D(CenterRight, InnerTop + HeaderHeight));
        Layout.SelectionPanel = FBox2D(FVector2D(CenterLeft, InnerTop + HeaderHeight + Gap),
            FVector2D(CenterRight, InnerBottom));
        Layout.MainPanel = Layout.SelectionPanel;
        Layout.CommandDeckPanel = FBox2D(FVector2D(Width - Gap - DeckWidth, InnerTop),
            FVector2D(Width - Gap, InnerBottom));
        Layout.StatusPanel = FBox2D(FVector2D(Edge, Top - 62.0f * Scale),
            FVector2D(Width - Edge, Top - 8.0f));
        Layout.bBottomBarVisible = Height >= 360;
        Layout.bMenuVisible = Layout.bBottomBarVisible &&
            Layout.MenuPanel.Max.X <= Width - Edge;
        // Do not render an unreadable or overlapping telemetry fragment.
        Layout.bResourceVisible = Layout.bBottomBarVisible &&
            Layout.ResourcePanel.GetSize().X >= 220.0f * Scale;
        Layout.bMinimapVisible = Layout.bBottomBarVisible && MapSize >= 100;
        Layout.bSelectionVisible = Layout.bBottomBarVisible && CenterRight - CenterLeft >= 180;
        Layout.bObjectiveVisible = Layout.bSelectionVisible;
        Layout.bCommandDeckVisible = Layout.bSelectionVisible;
        Layout.bStatusVisible = bHasStatusMessage && Layout.StatusPanel.Min.Y >= Edge;
        return Layout;
    }

    /** A stable keyboard target in the unobstructed battlefield. Reserve the
     * instruction region even when its message fades, so the aim never jumps.
     * Drawing, ground/entity traces and camera focus must use this same point. */
    [[nodiscard]] static FVector2D KeyboardTargetPoint(
        const FVector2D& ViewportSize, float HudScale, const FVector2D& Offset)
    {
        if (ViewportSize.ContainsNaN() || ViewportSize.X <= 0 || ViewportSize.Y <= 0)
            return FVector2D::ZeroVector;
        const auto Layout = Build(ViewportSize, HudScale, true);
        const double ClearHeight = Layout.bBottomBarVisible
            ? (Layout.bStatusVisible ? Layout.StatusPanel.Min.Y : Layout.BottomBar.Min.Y)
            : ViewportSize.Y;
        const FVector2D Center(ViewportSize.X * 0.5, ClearHeight * 0.5);
        const FVector2D Target = Center + (Offset.ContainsNaN() ? FVector2D::ZeroVector : Offset);
        const double Margin = FMath::Min(24.0, FMath::Min(ViewportSize.X, ClearHeight) * 0.25);
        // The reticle marks a battlefield point, so it must not come to rest on
        // drawn chrome. The upper band now carries the Menu control and the
        // resource strip, and a hard-left, hard-up aim used to clamp straight
        // onto the Menu button. Keep the top limit below whatever is visible up
        // there, while never pushing past the clear region's own lower bound.
        double TopLimit = Margin;
        if (Layout.bMenuVisible)
        {
            TopLimit = FMath::Max(TopLimit, Layout.MenuPanel.Max.Y + Margin);
        }
        if (Layout.bResourceVisible)
        {
            TopLimit = FMath::Max(TopLimit, Layout.ResourcePanel.Max.Y + Margin);
        }
        const double BottomLimit = ClearHeight - Margin;
        TopLimit = FMath::Min(TopLimit, BottomLimit);
        return FVector2D(FMath::Clamp(Target.X, Margin, ViewportSize.X - Margin),
            FMath::Clamp(Target.Y, TopLimit, BottomLimit));
    }

    /** True when a pointer position lies on any drawn chrome panel. */
    [[nodiscard]] bool IsPointerOnChrome(const FVector2D& ScreenPosition) const
    {
        return MainPanel.IsInsideOrOn(ScreenPosition) ||
               (bResourceVisible && ResourcePanel.IsInsideOrOn(ScreenPosition)) ||
               (bMenuVisible && MenuPanel.IsInsideOrOn(ScreenPosition)) ||
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
               (!bMenuVisible || !OverlapsExpanded(MenuPanel)) &&
               (!bBottomBarVisible || !OverlapsExpanded(BottomBar)) &&
               (!bObjectiveVisible || !OverlapsExpanded(ObjectivePanel)) &&
               (!bCommandDeckVisible || !OverlapsExpanded(CommandDeckPanel)) &&
               (!bStatusVisible || !OverlapsExpanded(StatusPanel)) &&
               (!bMinimapVisible || !OverlapsExpanded(MinimapPanel));
    }
};
