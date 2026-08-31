#pragma once

#include "CoreMinimal.h"

/** Shared screen geometry for the title entry and direct-connect modal. */
struct FEchoesOnlineFrontDoorLayout final
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    float ContentScale = 1.0f;
    float TextScale = 1.0f;
    FBox2D HostButton;
    FBox2D EndpointField;
    FBox2D JoinButton;
    FBox2D BackButton;
    FBox2D RetryButton;

    [[nodiscard]] static FEchoesOnlineFrontDoorLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesOnlineFrontDoorLayout Layout;
        Layout.Size.X = FMath::Min(
            FMath::Max(620.0f, ViewportSize.X - 60.0f),
            FMath::Clamp(ViewportSize.X * 0.54f, 760.0f, 980.0f));
        Layout.Size.Y = FMath::Min(
            FMath::Max(500.0f, ViewportSize.Y - 60.0f),
            FMath::Clamp(ViewportSize.Y * 0.62f, 560.0f, 680.0f));
        Layout.Origin = (ViewportSize - Layout.Size) * 0.5f;
        Layout.ContentScale = FMath::Clamp(
            FMath::Min(Layout.Size.X / 820.0f, Layout.Size.Y / 590.0f),
            0.76f,
            1.2f);
        Layout.TextScale = FMath::Clamp(HudScale, 0.75f, 1.35f) *
            Layout.ContentScale;

        const float ControlX = Layout.Origin.X + 46.0f;
        const float ControlWidth = Layout.Size.X - 92.0f;
        const float ControlHeight = 50.0f * Layout.ContentScale;
        const auto MakeControl = [&](float DesignY)
        {
            const FVector2D Minimum(
                ControlX,
                Layout.Origin.Y + DesignY * Layout.ContentScale);
            return FBox2D(
                Minimum,
                Minimum + FVector2D(ControlWidth, ControlHeight));
        };
        Layout.HostButton = MakeControl(172.0f);
        Layout.EndpointField = MakeControl(252.0f);
        Layout.JoinButton = MakeControl(326.0f);
        const FVector2D BackMinimum(
            ControlX,
            Layout.Origin.Y + Layout.Size.Y - 78.0f);
        Layout.BackButton = FBox2D(
            BackMinimum,
            BackMinimum + FVector2D(ControlWidth, 44.0f));
        Layout.RetryButton = MakeControl(314.0f);
        return Layout;
    }

    [[nodiscard]] static FBox2D BuildTitleEntry(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        const float Scale = FMath::Clamp(HudScale, 0.75f, 1.35f);
        const float Width = FMath::Min(360.0f * Scale,
                                      ViewportSize.X - 40.0f);
        const float Height = 42.0f * Scale;
        const FVector2D Minimum(
            ViewportSize.X - Width - 20.0f,
            20.0f);
        return FBox2D(Minimum, Minimum + FVector2D(Width, Height));
    }
};

/** Shared geometry for the local-only active online match menu. */
struct FEchoesOnlineLocalMenuLayout final
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    float ContentScale = 1.0f;
    float TextScale = 1.0f;
    FBox2D ResumeButton;
    FBox2D LeaveButton;

    [[nodiscard]] static FEchoesOnlineLocalMenuLayout Build(
        const FVector2D& ViewportSize,
        float HudScale)
    {
        FEchoesOnlineLocalMenuLayout Layout;
        Layout.Size.X = FMath::Min(
            FMath::Max(560.0f, ViewportSize.X - 60.0f),
            FMath::Clamp(ViewportSize.X * 0.48f, 640.0f, 820.0f));
        Layout.Size.Y = FMath::Min(
            FMath::Max(340.0f, ViewportSize.Y - 60.0f),
            FMath::Clamp(ViewportSize.Y * 0.44f, 390.0f, 500.0f));
        Layout.Origin = (ViewportSize - Layout.Size) * 0.5f;
        Layout.ContentScale = FMath::Clamp(
            FMath::Min(Layout.Size.X / 720.0f, Layout.Size.Y / 430.0f),
            0.78f,
            1.2f);
        Layout.TextScale = FMath::Clamp(HudScale, 0.75f, 1.35f) *
            Layout.ContentScale;
        const float Left = Layout.Origin.X + 42.0f;
        const float Width = Layout.Size.X - 84.0f;
        const float Height = 52.0f * Layout.ContentScale;
        const FVector2D ResumeMinimum(
            Left,
            Layout.Origin.Y + 214.0f * Layout.ContentScale);
        const FVector2D LeaveMinimum(
            Left,
            Layout.Origin.Y + 288.0f * Layout.ContentScale);
        Layout.ResumeButton = FBox2D(
            ResumeMinimum,
            ResumeMinimum + FVector2D(Width, Height));
        Layout.LeaveButton = FBox2D(
            LeaveMinimum,
            LeaveMinimum + FVector2D(Width, Height));
        return Layout;
    }
};
