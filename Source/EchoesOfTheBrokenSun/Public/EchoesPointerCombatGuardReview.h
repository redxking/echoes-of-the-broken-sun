#pragma once

#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

/** One bounded non-shipping camera/HUD configuration for pointer acceptance. */
struct FEchoesPointerCombatGuardReview final
{
    FString Variant = TEXT("Default");
    float HudScale = 1.0f;
    FVector CameraLocation = FVector(-2800.0f, -3300.0f, 100.0f);
    FVector2D CameraCenterTile = FVector2D(12.0f, 7.0f);
    float CameraZoom = 6500.0f;
    FIntPoint ExpectedViewport = FIntPoint(1600, 900);

    [[nodiscard]] static bool TryResolve(
        const FString& RequestedVariant,
        FEchoesPointerCombatGuardReview& OutConfiguration)
    {
        const FString Variant = RequestedVariant.IsEmpty()
            ? TEXT("Default")
            : RequestedVariant;
        OutConfiguration = FEchoesPointerCombatGuardReview{};
        if (Variant.Equals(TEXT("Default"), ESearchCase::IgnoreCase))
        {
            return true;
        }
        if (Variant.Equals(TEXT("Offset"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("Offset");
            OutConfiguration.HudScale = 1.0f;
            OutConfiguration.CameraLocation = FVector(-2750.0f, -3350.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(12.5f, 6.5f);
            OutConfiguration.CameraZoom = 6200.0f;
            return true;
        }
        if (Variant.Equals(TEXT("MinHud"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("MinHud");
            OutConfiguration.HudScale = 0.85f;
            OutConfiguration.CameraLocation = FVector(-2350.0f, -2325.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(16.5f, 16.75f);
            OutConfiguration.CameraZoom = 5500.0f;
            return true;
        }
        if (Variant.Equals(TEXT("MaxHud"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("MaxHud");
            OutConfiguration.HudScale = 1.35f;
            OutConfiguration.CameraLocation = FVector(-3550.0f, -4030.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(4.5f, -0.3f);
            OutConfiguration.CameraZoom = 9000.0f;
            return true;
        }
        if (Variant.Equals(TEXT("Compact"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("Compact");
            OutConfiguration.HudScale = 0.85f;
            OutConfiguration.CameraLocation = FVector(-2600.0f, -3500.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(14.0f, 5.0f);
            OutConfiguration.CameraZoom = 6500.0f;
            OutConfiguration.ExpectedViewport = FIntPoint(1280, 720);
            return true;
        }
        if (Variant.Equals(TEXT("Mac16x10"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("Mac16x10");
            OutConfiguration.HudScale = 1.0f;
            OutConfiguration.CameraLocation = FVector(-2800.0f, -3300.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(12.0f, 7.0f);
            OutConfiguration.CameraZoom = 6500.0f;
            OutConfiguration.ExpectedViewport = FIntPoint(1440, 900);
            return true;
        }
        if (Variant.Equals(TEXT("FullHD"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("FullHD");
            OutConfiguration.HudScale = 1.35f;
            OutConfiguration.CameraLocation = FVector(-2800.0f, -3300.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(12.0f, 7.0f);
            OutConfiguration.CameraZoom = 6500.0f;
            OutConfiguration.ExpectedViewport = FIntPoint(1920, 1080);
            return true;
        }
        return false;
    }

    [[nodiscard]] static bool TryFromCommandLine(
        FEchoesPointerCombatGuardReview& OutConfiguration,
        FString& OutRequestedVariant)
    {
        OutRequestedVariant.Reset();
        FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesPointerCombatGuardReviewVariant="),
            OutRequestedVariant);
        return TryResolve(OutRequestedVariant, OutConfiguration);
    }
};
