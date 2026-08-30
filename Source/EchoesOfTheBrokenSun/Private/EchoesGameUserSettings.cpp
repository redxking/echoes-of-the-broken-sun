#include "EchoesGameUserSettings.h"

#include "Engine/Engine.h"

namespace
{
constexpr float MinimumHudScale = 0.85f;
constexpr float MaximumHudScale = 1.35f;
constexpr float MinimumCameraScale = 0.5f;
constexpr float MaximumCameraScale = 2.0f;
constexpr float MinimumEffectsVolume = 0.0f;
constexpr float MaximumEffectsVolume = 1.0f;
}

UEchoesGameUserSettings* UEchoesGameUserSettings::Get()
{
    if (GEngine != nullptr)
    {
        return Cast<UEchoesGameUserSettings>(GEngine->GetGameUserSettings());
    }
    return nullptr;
}

void UEchoesGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    HudScale = 1.0f;
    bHighContrastHud = false;
    bReducedMotion = false;
    bReducedFlashing = false;
    bEdgePan = true;
    CameraPanSpeedScale = 1.0f;
    CameraZoomScale = 1.0f;
    EffectsVolume = 1.0f;
    bReducedDynamicRange = false;
}

void UEchoesGameUserSettings::ValidateSettings()
{
    Super::ValidateSettings();
    HudScale = FMath::Clamp(HudScale, MinimumHudScale, MaximumHudScale);
    CameraPanSpeedScale = FMath::Clamp(
        CameraPanSpeedScale,
        MinimumCameraScale,
        MaximumCameraScale);
    CameraZoomScale = FMath::Clamp(
        CameraZoomScale,
        MinimumCameraScale,
        MaximumCameraScale);
    EffectsVolume = FMath::Clamp(
        EffectsVolume,
        MinimumEffectsVolume,
        MaximumEffectsVolume);
}

float UEchoesGameUserSettings::GetHudScale() const
{
    return FMath::Clamp(HudScale, MinimumHudScale, MaximumHudScale);
}

void UEchoesGameUserSettings::SetHudScale(float NewScale)
{
    HudScale = FMath::Clamp(NewScale, MinimumHudScale, MaximumHudScale);
}

bool UEchoesGameUserSettings::IsHighContrastHudEnabled() const
{
    return bHighContrastHud;
}

void UEchoesGameUserSettings::SetHighContrastHudEnabled(bool bEnabled)
{
    bHighContrastHud = bEnabled;
}

bool UEchoesGameUserSettings::IsReducedMotionEnabled() const
{
    return bReducedMotion;
}

void UEchoesGameUserSettings::SetReducedMotionEnabled(bool bEnabled)
{
    bReducedMotion = bEnabled;
}

bool UEchoesGameUserSettings::IsReducedFlashingEnabled() const
{
    return bReducedFlashing;
}

void UEchoesGameUserSettings::SetReducedFlashingEnabled(bool bEnabled)
{
    bReducedFlashing = bEnabled;
}

bool UEchoesGameUserSettings::IsEdgePanEnabled() const
{
    return bEdgePan;
}

void UEchoesGameUserSettings::SetEdgePanEnabled(bool bEnabled)
{
    bEdgePan = bEnabled;
}

float UEchoesGameUserSettings::GetCameraPanSpeedScale() const
{
    return FMath::Clamp(
        CameraPanSpeedScale,
        MinimumCameraScale,
        MaximumCameraScale);
}

void UEchoesGameUserSettings::SetCameraPanSpeedScale(float NewScale)
{
    CameraPanSpeedScale = FMath::Clamp(
        NewScale,
        MinimumCameraScale,
        MaximumCameraScale);
}

float UEchoesGameUserSettings::GetCameraZoomScale() const
{
    return FMath::Clamp(
        CameraZoomScale,
        MinimumCameraScale,
        MaximumCameraScale);
}

void UEchoesGameUserSettings::SetCameraZoomScale(float NewScale)
{
    CameraZoomScale = FMath::Clamp(
        NewScale,
        MinimumCameraScale,
        MaximumCameraScale);
}

float UEchoesGameUserSettings::GetEffectsVolume() const
{
    return FMath::Clamp(
        EffectsVolume,
        MinimumEffectsVolume,
        MaximumEffectsVolume);
}

void UEchoesGameUserSettings::SetEffectsVolume(float NewVolume)
{
    EffectsVolume = FMath::Clamp(
        NewVolume,
        MinimumEffectsVolume,
        MaximumEffectsVolume);
}

bool UEchoesGameUserSettings::IsReducedDynamicRangeEnabled() const
{
    return bReducedDynamicRange;
}

void UEchoesGameUserSettings::SetReducedDynamicRangeEnabled(bool bEnabled)
{
    bReducedDynamicRange = bEnabled;
}
