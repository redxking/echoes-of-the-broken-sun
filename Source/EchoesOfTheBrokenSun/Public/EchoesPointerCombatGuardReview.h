#pragma once

#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

/** One bounded non-shipping camera/HUD configuration for pointer acceptance. */
struct FEchoesPointerCombatGuardReview final
{
    FString Variant = TEXT("Default");
    float HudScale = 1.0f;
    FVector CameraLocation = FVector(-2200.0f, -2100.0f, 100.0f);
    FVector2D CameraCenterTile = FVector2D(18.0f, 19.0f);
    float CameraZoom = 5800.0f;

    [[nodiscard]] static bool TryResolve(
        const FString& RequestedVariant,
        FEchoesPointerCombatGuardReview& OutConfiguration)
    {
        const FString Variant = RequestedVariant.IsEmpty()
            ? TEXT("Default")
            : RequestedVariant;
        if (Variant.Equals(TEXT("Default"), ESearchCase::IgnoreCase))
        {
            OutConfiguration = FEchoesPointerCombatGuardReview{};
            return true;
        }
        if (Variant.Equals(TEXT("Offset"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("Offset");
            OutConfiguration.HudScale = 1.0f;
            OutConfiguration.CameraLocation = FVector(-2300.0f, -2000.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(17.0f, 20.0f);
            OutConfiguration.CameraZoom = 6200.0f;
            return true;
        }
        if (Variant.Equals(TEXT("MinHud"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("MinHud");
            OutConfiguration.HudScale = 0.85f;
            OutConfiguration.CameraLocation = FVector(-2300.0f, -1900.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(17.0f, 21.0f);
            OutConfiguration.CameraZoom = 5000.0f;
            return true;
        }
        if (Variant.Equals(TEXT("MaxHud"), ESearchCase::IgnoreCase))
        {
            OutConfiguration.Variant = TEXT("MaxHud");
            OutConfiguration.HudScale = 1.35f;
            OutConfiguration.CameraLocation = FVector(-4000.0f, -3500.0f, 100.0f);
            OutConfiguration.CameraCenterTile = FVector2D(0.0f, 5.0f);
            OutConfiguration.CameraZoom = 6500.0f;
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
