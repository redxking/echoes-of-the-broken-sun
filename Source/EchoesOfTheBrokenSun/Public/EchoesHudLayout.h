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
    // The pixel space every rect above is expressed in. A consumer that draws
    // or hit-tests in a different space is the resolution-mismatch defect that
    // clipped the whole console away, so the space travels with the rects.
    FVector2D ViewSize = FVector2D::ZeroVector;

    /** The console's physical scale on a given surface: the accessibility
     * HUD scale grown by the viewport DPI curve above 1080 lines, so a 1440 or
     * Retina surface does not draw the 720-line console at 720-line pixel
     * size (REL-UI-013). Below 1080 lines the curve is not applied: the
     * 720-line profile is authored at 1.0 and is the minimum. Every consumer
     * of a layout at runtime (widget, camera pawn, controller) must pass this
     * value, never the raw setting, or their rects disagree. */
    [[nodiscard]] static float EffectiveScale(float HudScale, float ViewportDpi)
    {
        return FMath::Clamp(HudScale * FMath::Max(1.0f, ViewportDpi), 0.8f, 1.5f);
    }

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
        // 96 units of text at scale, the objective header takes 92 (a 14-point
        // title over two 16-point lines with its padding), and three gaps of
        // 14 sit between and around them. The console panels do not scroll
        // (owner direction, 2026-09-11): 272 gives the selection card 142
        // units at 100%, which holds a unit's vitals and role or a producer's
        // queue and its controls without a scroll bar. Shrinking below that sum left
        // the centre panel too short for a single selected unit at 150% on a
        // 720-line display. The lower-half ceiling still wins over the floor.
        const float SelectionTextHeight = 96.0f * Scale;
        const float HeaderTextHeight = 92.0f * Scale;
        const float MinBarHeight = SelectionTextHeight + HeaderTextHeight + 3.0f * Gap;
        const float BarHeight = FMath::Clamp(
            FMath::Min(272.0f * Scale, Height * 0.38f),
            FMath::Min(MinBarHeight, Height * 0.5f),
            Height * 0.5f);
        const float Top = Height - BarHeight;
        const float InnerTop = Top + Gap;
        const float InnerBottom = Height - Gap;
        const float HeaderHeight = 92.0f * Scale;
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
        // 540 left the logistics value touching the clip edge at 2560x1440,
        // where this cap is the binding constraint; the other two terms still
        // bound the panel on narrow surfaces.
        // At 1280 wide and 1.5 scale this fraction, not the cap above, is the
        // binding term, and 0.58 left the logistics readout overflowing its
        // panel by about three units. The panel still starts well clear of the
        // reserved menu bounds at the widened fraction.
        const float ResourceWidth = FMath::Min(FMath::Min(600.0f * Scale, Width * 0.62f),
            ResourceRight - ResourceLeftLimit);
        // Leave the existing upper-right tutorial skip control unobstructed.
        // Three readable source-backed lines require more than the original
        // 64-dip proposal: one resource row plus faction and match/research
        // context. This stays clear of the upper-right tutorial affordance.
        //
        // The ledger clips to its own bounds, so a height equal to the sum of
        // its rows cuts the glyph tips off the first and last of them. An
        // 18-point value row over two 14-point context rows measures about 84
        // units with no padding at all; 104 kept the border off the text at
        // every scale in the accessibility range. With the labels stacked
        // over their values the ledger needs 89 at 100%, and 96 leaves the
        // deployment frame the 140-unit silhouette room it needs above the
        // 272-unit console (EchoesOrthographicCameraTest).
        // The ledger's headroom belongs to the edge it already reserves, not to the
        // battlefield below it. KeyboardTargetPoint halves StatusPanel.Min.Y to place
        // the deployment framing centre, and the framed headquarters' silhouette
        // reaches to that centre minus 70 -- which this panel's lower edge had grown
        // onto. 2 is ConfineToView's own just-inside-the-edge inset. The authored
        // 104*Scale height is unchanged.
        constexpr float ResourceTop = 2.0f;
        Layout.ResourcePanel = FBox2D(FVector2D(ResourceRight - ResourceWidth, ResourceTop),
            FVector2D(ResourceRight, ResourceTop + 96.0f * Scale));
        Layout.MinimapPanel = FBox2D(FVector2D(Gap, InnerBottom - MapSize),
            FVector2D(Gap + MapSize, InnerBottom));
        Layout.ObjectivePanel = FBox2D(FVector2D(CenterLeft, InnerTop),
            FVector2D(CenterRight, InnerTop + HeaderHeight));
        Layout.SelectionPanel = FBox2D(FVector2D(CenterLeft, InnerTop + HeaderHeight + Gap),
            FVector2D(CenterRight, InnerBottom));
        Layout.MainPanel = Layout.SelectionPanel;
        Layout.CommandDeckPanel = FBox2D(FVector2D(Width - Gap - DeckWidth, InnerTop),
            FVector2D(Width - Gap, InnerBottom));
        // A full status sentence wraps to two lines once the accessibility
        // scale grows the text on a 1280-wide surface, and 62 units held only
        // one of them: the second line was cut by the panel's own lower edge.
        // This band's top edge is the battlefield floor the camera frames against,
        // so it may not rise into the deployment frame (the +6 on Min.Y is that
        // floor). Its bottom edge must stay clear of the console bar: ConfineToView
        // lands BottomBar at Top-2, so a Max.Y of Top-2 touched the bar with zero
        // clearance at every resolution and scale (SPEC-UI-007, REL-UI-004.FAIL).
        // Top-8 keeps the authored 6-unit gap; the band is 92*Scale-14 tall and
        // its text is top-anchored, so only empty space below the text is lost.
        Layout.StatusPanel = FBox2D(FVector2D(Edge, Top - 92.0f * Scale + 6.0f),
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
        Layout.ViewSize = FVector2D(Width, Height);
        Layout.ConfineToView();
        return Layout;
    }

    /** Intersect every panel with the view and drop whatever no longer fits.
     *
     * Build sizes panels from whatever the caller passed. When that size
     * disagreed with the surface actually being drawn - a borderless 2560x1440
     * window while the engine reported 1280x720 - the resource ledger,
     * objectives, selection panel, command card and results actions were all
     * truncated off screen and pointer input reached nothing
     * (BuildArtifacts/Evidence/connected-input-20260909T204502Z, defect 2).
     * Clamping here makes that unrepresentable for every consumer at once: a
     * panel is either wholly inside the space its rects are expressed in, or it
     * is not visible. A panel clamped below its readable minimum is hidden
     * rather than drawn as a sliver, because a truncated command card that
     * still hit-tests is worse than an absent one. */
    void ConfineToView()
    {
        const double Width = FMath::Max(1.0, ViewSize.X);
        const double Height = FMath::Max(1.0, ViewSize.Y);
        // A panel narrower or shorter than this cannot carry its own label, so
        // it is withdrawn instead of clipped to an unreadable strip.
        constexpr double MinReadable = 24.0;
        const auto Confine = [Width, Height](FBox2D& Rect, bool& bVisible)
        {
            if (!bVisible)
            {
                return;
            }
            if (Rect.Min.ContainsNaN() || Rect.Max.ContainsNaN() ||
                Rect.Max.X <= Rect.Min.X || Rect.Max.Y <= Rect.Min.Y)
            {
                bVisible = false;
                return;
            }
            // Slide an overhanging panel back into view before clamping.
            // Clamping alone shrinks it against the edge it overhangs, which
            // is what cut the top row off the resource ledger at 1.5 scale:
            // the panel kept its position and lost its first line instead of
            // moving down. Translation preserves the authored size, so a panel
            // is only ever trimmed when it genuinely cannot fit.
            const FVector2D Size = Rect.GetSize();
            FVector2D Origin = Rect.Min;
            // A panel landed flush on the viewport edge loses the outer half
            // of its own border stroke, which reads as a clipped panel even
            // though the rect is inside. Land it just inside instead.
            constexpr double EdgeInset = 2.0;
            const double InsetX = Size.X + 2.0 * EdgeInset <= Width ? EdgeInset : 0.0;
            const double InsetY = Size.Y + 2.0 * EdgeInset <= Height ? EdgeInset : 0.0;
            Origin.X = Size.X <= Width ? FMath::Clamp(Origin.X, InsetX, Width - Size.X - InsetX) : 0.0;
            Origin.Y = Size.Y <= Height ? FMath::Clamp(Origin.Y, InsetY, Height - Size.Y - InsetY) : 0.0;
            Rect = FBox2D(Origin, Origin + Size);
            Rect.Min.X = FMath::Clamp(Rect.Min.X, 0.0, Width);
            Rect.Min.Y = FMath::Clamp(Rect.Min.Y, 0.0, Height);
            Rect.Max.X = FMath::Clamp(Rect.Max.X, 0.0, Width);
            Rect.Max.Y = FMath::Clamp(Rect.Max.Y, 0.0, Height);
            if (Rect.GetSize().X < MinReadable || Rect.GetSize().Y < MinReadable)
            {
                bVisible = false;
            }
        };
        Confine(BottomBar, bBottomBarVisible);
        Confine(MenuPanel, bMenuVisible);
        Confine(ResourcePanel, bResourceVisible);
        Confine(MinimapPanel, bMinimapVisible);
        Confine(ObjectivePanel, bObjectiveVisible);
        Confine(SelectionPanel, bSelectionVisible);
        Confine(CommandDeckPanel, bCommandDeckVisible);
        Confine(StatusPanel, bStatusVisible);
        MainPanel = SelectionPanel;
    }

    /** Every rect a consumer may draw or hit-test, paired with its flag. Used
     * by automation to assert containment without restating the panel list. */
    [[nodiscard]] TArray<TPair<FBox2D, bool>> VisiblePanels() const
    {
        return {
            {BottomBar, bBottomBarVisible},
            {MenuPanel, bMenuVisible},
            {ResourcePanel, bResourceVisible},
            {MinimapPanel, bMinimapVisible},
            {ObjectivePanel, bObjectiveVisible},
            {SelectionPanel, bSelectionVisible},
            {CommandDeckPanel, bCommandDeckVisible},
            {StatusPanel, bStatusVisible}};
    }

    /** True when no visible panel leaves the space its rects are built in. */
    [[nodiscard]] bool IsFullyOnScreen() const
    {
        for (const TPair<FBox2D, bool>& Panel : VisiblePanels())
        {
            if (!Panel.Value)
            {
                continue;
            }
            if (Panel.Key.Min.X < -KINDA_SMALL_NUMBER ||
                Panel.Key.Min.Y < -KINDA_SMALL_NUMBER ||
                Panel.Key.Max.X > ViewSize.X + KINDA_SMALL_NUMBER ||
                Panel.Key.Max.Y > ViewSize.Y + KINDA_SMALL_NUMBER)
            {
                return false;
            }
        }
        return true;
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
