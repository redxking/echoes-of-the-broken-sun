#pragma once

#include "CoreMinimal.h"

/** Shared screen-space geometry for the offline skirmish setup overlay. */
struct FEchoesSkirmishSetupOverlayLayout final
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    float ContentScale = 1.0f;
    float TextScale = 1.0f;
    FBox2D SettingRows[9];
    FBox2D SettingDecrease[9];
    FBox2D SettingIncrease[9];
    FBox2D AssistedBannerBox;
    FBox2D ReviewButton;

    [[nodiscard]] static FEchoesSkirmishSetupOverlayLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesSkirmishSetupOverlayLayout Layout;
        Layout.Size.X = FMath::Min(
            FMath::Max(760.0f, ViewportSize.X - 60.0f),
            FMath::Clamp(ViewportSize.X * 0.66f, 920.0f, 1260.0f));
        Layout.Size.Y = FMath::Min(
            FMath::Max(640.0f, ViewportSize.Y - 40.0f),
            FMath::Clamp(ViewportSize.Y * 0.86f, 680.0f, 860.0f));
        Layout.Origin = (ViewportSize - Layout.Size) * 0.5f;
        Layout.ContentScale = FMath::Clamp(
            FMath::Min(Layout.Size.X / 980.0f, Layout.Size.Y / 740.0f),
            0.74f,
            1.2f);
        Layout.TextScale = FMath::Clamp(HudScale, 0.75f, 1.35f) *
            Layout.ContentScale;

        for (int32 Row = 0; Row < 9; ++Row)
        {
            const float RowTop = Layout.Origin.Y +
                (104.0f + Row * 36.0f) * Layout.ContentScale;
            const FVector2D RowMin(
                Layout.Origin.X + 38.0f,
                RowTop - 4.0f * Layout.ContentScale);
            const FVector2D RowMax(
                Layout.Origin.X + Layout.Size.X - 38.0f,
                RowTop + 28.0f * Layout.ContentScale);
            Layout.SettingRows[Row] = FBox2D(RowMin, RowMax);
            const float MidX = (RowMin.X + RowMax.X) * 0.5f;
            Layout.SettingDecrease[Row] = FBox2D(
                RowMin,
                FVector2D(MidX, RowMax.Y));
            Layout.SettingIncrease[Row] = FBox2D(
                FVector2D(MidX, RowMin.Y),
                RowMax);
        }

        const float BannerTop = Layout.Origin.Y +
            (104.0f + 9 * 36.0f + 4.0f) * Layout.ContentScale;
        const FVector2D BannerMin(
            Layout.Origin.X + 38.0f,
            BannerTop);
        Layout.AssistedBannerBox = FBox2D(
            BannerMin,
            FVector2D(
                Layout.Origin.X + Layout.Size.X - 38.0f,
                BannerTop + 30.0f * Layout.ContentScale));

        const FVector2D ReviewMin(
            Layout.Origin.X + 46.0f,
            Layout.Origin.Y + Layout.Size.Y - 64.0f);
        Layout.ReviewButton = FBox2D(
            ReviewMin,
            FVector2D(
                Layout.Origin.X + Layout.Size.X - 46.0f,
                ReviewMin.Y + 42.0f));
        return Layout;
    }
};

/** Shared screen-space geometry for the skirmish deployment summary. */
struct FEchoesSkirmishSummaryOverlayLayout final
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    float ContentScale = 1.0f;
    float TextScale = 1.0f;
    FBox2D BackButton;
    FBox2D DeployButton;

    [[nodiscard]] static FEchoesSkirmishSummaryOverlayLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesSkirmishSummaryOverlayLayout Layout;
        Layout.Size.X = FMath::Min(
            FMath::Max(720.0f, ViewportSize.X - 60.0f),
            FMath::Clamp(ViewportSize.X * 0.62f, 840.0f, 1160.0f));
        Layout.Size.Y = FMath::Min(
            FMath::Max(600.0f, ViewportSize.Y - 40.0f),
            FMath::Clamp(ViewportSize.Y * 0.82f, 660.0f, 840.0f));
        Layout.Origin = (ViewportSize - Layout.Size) * 0.5f;
        Layout.ContentScale = FMath::Clamp(
            FMath::Min(Layout.Size.X / 920.0f, Layout.Size.Y / 720.0f),
            0.74f,
            1.2f);
        Layout.TextScale = FMath::Clamp(HudScale, 0.75f, 1.35f) *
            Layout.ContentScale;

        const FVector2D BackMin(
            Layout.Origin.X + 54.0f,
            Layout.Origin.Y + Layout.Size.Y - 118.0f);
        Layout.BackButton = FBox2D(
            BackMin,
            BackMin + FVector2D(
                FMath::Min(320.0f * Layout.ContentScale,
                           Layout.Size.X - 108.0f),
                36.0f * Layout.ContentScale));
        const FVector2D DeployMin(
            Layout.Origin.X + 46.0f,
            Layout.Origin.Y + Layout.Size.Y - 68.0f);
        Layout.DeployButton = FBox2D(
            DeployMin,
            FVector2D(
                Layout.Origin.X + Layout.Size.X - 46.0f,
                DeployMin.Y + 42.0f));
        return Layout;
    }
};

/** Shared screen-space geometry for the in-match pause overlay. */
struct FEchoesPauseOverlayLayout final
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    float ContentScale = 1.0f;
    float TextScale = 1.0f;
    FBox2D ResumeButton;
    FBox2D RestartButton;
    FBox2D ReturnButton;
    FBox2D PrimaryButton;

    [[nodiscard]] static FEchoesPauseOverlayLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesPauseOverlayLayout Layout;
        Layout.Size.X = FMath::Min(
            FMath::Max(620.0f, ViewportSize.X - 60.0f),
            FMath::Clamp(ViewportSize.X * 0.50f, 720.0f, 980.0f));
        Layout.Size.Y = FMath::Min(
            FMath::Max(520.0f, ViewportSize.Y - 60.0f),
            FMath::Clamp(ViewportSize.Y * 0.64f, 560.0f, 700.0f));
        Layout.Origin = (ViewportSize - Layout.Size) * 0.5f;
        Layout.ContentScale = FMath::Clamp(
            FMath::Min(Layout.Size.X / 780.0f, Layout.Size.Y / 590.0f),
            0.76f,
            1.2f);
        Layout.TextScale = FMath::Clamp(HudScale, 0.75f, 1.35f) *
            Layout.ContentScale;

        const float ControlX = Layout.Origin.X + 32.0f;
        const float ControlWidth = Layout.Size.X - 64.0f;
        const float ControlHeight = 28.0f * Layout.ContentScale;
        const auto MakeControl = [&](float DesignY)
        {
            const FVector2D Min(
                ControlX,
                Layout.Origin.Y + DesignY * Layout.ContentScale);
            return FBox2D(
                Min,
                Min + FVector2D(ControlWidth, ControlHeight));
        };
        Layout.ResumeButton = MakeControl(150.0f);
        Layout.RestartButton = MakeControl(180.0f);
        Layout.ReturnButton = MakeControl(210.0f);
        const FVector2D PrimaryMin(
            Layout.Origin.X + 42.0f,
            Layout.Origin.Y + Layout.Size.Y - 76.0f);
        Layout.PrimaryButton = FBox2D(
            PrimaryMin,
            FVector2D(
                Layout.Origin.X + Layout.Size.X - 42.0f,
                PrimaryMin.Y + 42.0f));
        return Layout;
    }
};

/** Shared screen-space geometry for the match-result overlay. */
struct FEchoesResultOverlayLayout final
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    float ContentScale = 1.0f;
    float TextScale = 1.0f;
    FBox2D FullButton;
    FBox2D PrimaryButton;
    FBox2D RestartButton;

    [[nodiscard]] static FEchoesResultOverlayLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesResultOverlayLayout Layout;
        Layout.Size.X = FMath::Min(
            FMath::Max(620.0f, ViewportSize.X - 60.0f),
            FMath::Clamp(ViewportSize.X * 0.54f, 760.0f, 1080.0f));
        Layout.Size.Y = FMath::Min(
            FMath::Max(390.0f, ViewportSize.Y - 60.0f),
            FMath::Clamp(ViewportSize.Y * 0.48f, 430.0f, 590.0f));
        Layout.Origin = (ViewportSize - Layout.Size) * 0.5f;
        Layout.ContentScale = FMath::Clamp(
            FMath::Min(Layout.Size.X / 820.0f, Layout.Size.Y / 460.0f),
            0.76f,
            1.25f);
        Layout.TextScale = FMath::Clamp(HudScale, 0.75f, 1.35f) *
            Layout.ContentScale;

        const FVector2D FullMin(
            Layout.Origin.X + 44.0f,
            Layout.Origin.Y + Layout.Size.Y - 82.0f);
        const FVector2D FullMax(
            Layout.Origin.X + Layout.Size.X - 44.0f,
            FullMin.Y + 46.0f);
        Layout.FullButton = FBox2D(FullMin, FullMax);
        constexpr float Gap = 12.0f;
        const float SplitX = (FullMin.X + FullMax.X - Gap) * 0.5f;
        Layout.PrimaryButton = FBox2D(
            FullMin,
            FVector2D(SplitX, FullMax.Y));
        Layout.RestartButton = FBox2D(
            FVector2D(SplitX + Gap, FullMin.Y),
            FullMax);
        return Layout;
    }
};
