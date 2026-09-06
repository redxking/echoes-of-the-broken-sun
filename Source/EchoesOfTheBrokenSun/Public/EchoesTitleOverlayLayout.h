#pragma once

#include "CoreMinimal.h"

/**
 * Shared pointer geometry for the campaign title menu, the mission briefing,
 * and the online lobby. The HUD draws FROM these boxes and the controller
 * hit-tests AGAINST them, so a control can never be visible without being
 * clickable. Panel math is the single source consumed by both sides.
 */
struct FEchoesTitleOverlayFacts final
{
    bool bContinueAvailable = false;
    bool bNewCampaignAvailable = false;
    bool bRestoreAvailable = false;
};

struct FEchoesTitleOverlayLayout final
{
    FBox2D Panel = FBox2D(ForceInit);
    FBox2D OperationButton = FBox2D(ForceInit);
    FBox2D FactionButton = FBox2D(ForceInit);
    FBox2D ContinueButton = FBox2D(ForceInit);
    FBox2D NewCampaignButton = FBox2D(ForceInit);
    FBox2D RestoreButton = FBox2D(ForceInit);
    FBox2D OpenBriefButton = FBox2D(ForceInit);
    float ContentScale = 1.0f;
    bool bContinueVisible = false;
    bool bNewCampaignVisible = false;
    bool bRestoreVisible = false;

    /** Historical panel geometry retained for explicit-position fixtures. */
    static void BuildPanel(
        const FVector2D& ViewportSize,
        FBox2D& OutPanel,
        float& OutContentScale)
    {
        const float Width = FMath::Max(1.0f, ViewportSize.X);
        const float Height = FMath::Max(1.0f, ViewportSize.Y);
        const float PanelWidth = FMath::Min(
            FMath::Max(700.0f, Width - 60.0f),
            FMath::Clamp(Width * 0.60f, 860.0f, 1220.0f));
        const float PanelHeight = FMath::Min(
            FMath::Max(560.0f, Height - 60.0f),
            FMath::Clamp(Height * 0.72f, 620.0f, 760.0f));
        const float Left = (Width - PanelWidth) * 0.5f;
        const float Top = (Height - PanelHeight) * 0.5f;
        OutPanel = FBox2D(
            FVector2D(Left, Top),
            FVector2D(Left + PanelWidth, Top + PanelHeight));
        OutContentScale = FMath::Clamp(
            FMath::Min(PanelWidth / 940.0f, PanelHeight / 650.0f),
            0.76f,
            1.22f);
    }

    [[nodiscard]] static FEchoesTitleOverlayLayout Build(
        const FVector2D& ViewportSize,
        const FEchoesTitleOverlayFacts& Facts)
    {
        FEchoesTitleOverlayLayout Layout;
        BuildPanel(ViewportSize, Layout.Panel, Layout.ContentScale);
        const float Left = Layout.Panel.Min.X;
        const float Top = Layout.Panel.Min.Y;
        const float PanelWidth = Layout.Panel.GetSize().X;
        const float PanelHeight = Layout.Panel.GetSize().Y;
        const float Scale = Layout.ContentScale;

        // The two control rows occupy exactly the band the operation and
        // campaign control text used before, so no other content moves.
        const float RowLeft = Left + 48.0f;
        const float RowRight = Left + PanelWidth - 48.0f;
        const float RowHeight = 26.0f * Scale;
        const auto Split = [](const FBox2D& Row, int32 Index, int32 Count,
                              float Gap)
        {
            const float Usable =
                Row.GetSize().X - Gap * static_cast<float>(Count - 1);
            const float SegmentWidth = Usable / static_cast<float>(Count);
            const float SegmentLeft =
                Row.Min.X + static_cast<float>(Index) * (SegmentWidth + Gap);
            return FBox2D(
                FVector2D(SegmentLeft, Row.Min.Y),
                FVector2D(SegmentLeft + SegmentWidth, Row.Max.Y));
        };

        const float OperationTop = Top + 266.0f * Scale;
        const FBox2D OperationRow(
            FVector2D(RowLeft, OperationTop),
            FVector2D(RowRight, OperationTop + RowHeight));
        Layout.OperationButton = Split(OperationRow, 0, 2, 12.0f * Scale);
        Layout.FactionButton = Split(OperationRow, 1, 2, 12.0f * Scale);

        const float CampaignTop = Top + 298.0f * Scale;
        const FBox2D CampaignRow(
            FVector2D(RowLeft, CampaignTop),
            FVector2D(RowRight, CampaignTop + RowHeight));
        Layout.ContinueButton = Split(CampaignRow, 0, 3, 12.0f * Scale);
        Layout.NewCampaignButton = Split(CampaignRow, 1, 3, 12.0f * Scale);
        Layout.RestoreButton = Split(CampaignRow, 2, 3, 12.0f * Scale);
        Layout.bContinueVisible = Facts.bContinueAvailable;
        Layout.bNewCampaignVisible = Facts.bNewCampaignAvailable;
        Layout.bRestoreVisible = Facts.bRestoreAvailable;

        // Exactly the existing bottom action bar.
        Layout.OpenBriefButton = FBox2D(
            FVector2D(Left + 48.0f, Top + PanelHeight - 82.0f),
            FVector2D(Left + PanelWidth - 48.0f, Top + PanelHeight - 36.0f));
        return Layout;
    }
};

/** Mission-briefing geometry: change-operation and deploy share the bar. */
struct FEchoesBriefingOverlayLayout final
{
    FBox2D Panel = FBox2D(ForceInit);
    FBox2D OperationButton = FBox2D(ForceInit);
    FBox2D DeployButton = FBox2D(ForceInit);
    float ContentScale = 1.0f;

    static void BuildPanel(
        const FVector2D& ViewportSize,
        FBox2D& OutPanel,
        float& OutContentScale)
    {
        const float Width = FMath::Max(1.0f, ViewportSize.X);
        const float Height = FMath::Max(1.0f, ViewportSize.Y);
        const float PanelWidth = FMath::Min(
            FMath::Max(620.0f, Width - 60.0f),
            FMath::Clamp(Width * 0.62f, 880.0f, 1280.0f));
        const float PanelHeight = FMath::Min(
            FMath::Max(500.0f, Height - 60.0f),
            FMath::Clamp(Height * 0.74f, 620.0f, 780.0f));
        const float Left = (Width - PanelWidth) * 0.5f;
        const float Top = (Height - PanelHeight) * 0.5f;
        OutPanel = FBox2D(
            FVector2D(Left, Top),
            FVector2D(Left + PanelWidth, Top + PanelHeight));
        OutContentScale = FMath::Clamp(
            FMath::Min(PanelWidth / 980.0f, PanelHeight / 680.0f),
            0.74f,
            1.20f);
    }

    [[nodiscard]] static FEchoesBriefingOverlayLayout Build(
        const FVector2D& ViewportSize)
    {
        FEchoesBriefingOverlayLayout Layout;
        BuildPanel(ViewportSize, Layout.Panel, Layout.ContentScale);
        const float Left = Layout.Panel.Min.X;
        const float Top = Layout.Panel.Min.Y;
        const float PanelWidth = Layout.Panel.GetSize().X;
        const float PanelHeight = Layout.Panel.GetSize().Y;

        // Exactly the existing bottom action bar, split so both actions the
        // bar advertises are separately clickable.
        const FBox2D Bar(
            FVector2D(Left + 42.0f, Top + PanelHeight - 74.0f),
            FVector2D(Left + PanelWidth - 42.0f, Top + PanelHeight - 32.0f));
        const float OperationWidth = FMath::Min(
            300.0f * Layout.ContentScale,
            Bar.GetSize().X * 0.40f);
        Layout.OperationButton = FBox2D(
            Bar.Min,
            FVector2D(Bar.Min.X + OperationWidth, Bar.Max.Y));
        Layout.DeployButton = FBox2D(
            FVector2D(Bar.Min.X + OperationWidth + 10.0f, Bar.Min.Y),
            Bar.Max);
        return Layout;
    }
};

/**
 * Online lobby geometry. Only the ready control is claimed here; the cancel
 * path belongs to the Network lane's session semantics and is deliberately
 * left unbound (recorded, not silently skipped).
 */
struct FEchoesLobbyOverlayLayout final
{
    FBox2D Panel = FBox2D(ForceInit);
    FBox2D ReadyButton = FBox2D(ForceInit);

    [[nodiscard]] static FEchoesLobbyOverlayLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesLobbyOverlayLayout Layout;
        const float Width = FMath::Max(1.0f, ViewportSize.X);
        const float Height = FMath::Max(1.0f, ViewportSize.Y);
        const float LobbyWidth = FMath::Min(Width - 80.0f, 720.0f);
        constexpr float LobbyHeight = 286.0f;
        const float LobbyX = (Width - LobbyWidth) * 0.5f;
        const float LobbyY = (Height - LobbyHeight) * 0.5f;
        Layout.Panel = FBox2D(
            FVector2D(LobbyX, LobbyY),
            FVector2D(LobbyX + LobbyWidth, LobbyY + LobbyHeight));
        const float Scale = FMath::Clamp(HudScale, 0.85f, 1.35f);
        // Anchored on the drawn "[ENTER] READY AND START MATCH" line.
        Layout.ReadyButton = FBox2D(
            FVector2D(LobbyX + 24.0f, LobbyY + 176.0f),
            FVector2D(
                LobbyX + LobbyWidth - 24.0f,
                LobbyY + 176.0f + 34.0f * Scale));
        return Layout;
    }
};
