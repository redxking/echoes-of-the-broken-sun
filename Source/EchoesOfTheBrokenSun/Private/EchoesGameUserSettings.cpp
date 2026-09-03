#include "EchoesGameUserSettings.h"

#include "Engine/Engine.h"

namespace
{
// ACC-002 requires a HUD scale range of 80-150%. These are the authoritative
// bounds for the persisted setting; presentation layers must not clamp tighter.
constexpr float MinimumHudScale = 0.80f;
constexpr float MaximumHudScale = 1.50f;
constexpr float MinimumCameraScale = 0.5f;
constexpr float MaximumCameraScale = 2.0f;
constexpr float MinimumEffectsVolume = 0.0f;
constexpr float MaximumEffectsVolume = 1.0f;

// A non-finite persisted volume is treated as silence rather than passed
// through: the mix fails closed rather than emitting an unbounded gain.
[[nodiscard]] float ClampVolume(float Value)
{
    if (!FMath::IsFinite(Value))
    {
        return 0.0f;
    }
    return FMath::Clamp(Value, MinimumEffectsVolume, MaximumEffectsVolume);
}
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
    MasterVolume = 1.0f;
    MusicVolume = 1.0f;
    DialogueVolume = 1.0f;
    InterfaceVolume = 1.0f;
    AmbienceVolume = 1.0f;
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
    MasterVolume = ClampVolume(MasterVolume);
    MusicVolume = ClampVolume(MusicVolume);
    DialogueVolume = ClampVolume(DialogueVolume);
    InterfaceVolume = ClampVolume(InterfaceVolume);
    AmbienceVolume = ClampVolume(AmbienceVolume);
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

float UEchoesGameUserSettings::GetMasterVolume() const
{
    return ClampVolume(MasterVolume);
}

void UEchoesGameUserSettings::SetMasterVolume(float NewVolume)
{
    MasterVolume = ClampVolume(NewVolume);
}

float UEchoesGameUserSettings::GetMusicVolume() const
{
    return ClampVolume(MusicVolume);
}

void UEchoesGameUserSettings::SetMusicVolume(float NewVolume)
{
    MusicVolume = ClampVolume(NewVolume);
}

float UEchoesGameUserSettings::GetDialogueVolume() const
{
    return ClampVolume(DialogueVolume);
}

void UEchoesGameUserSettings::SetDialogueVolume(float NewVolume)
{
    DialogueVolume = ClampVolume(NewVolume);
}

float UEchoesGameUserSettings::GetInterfaceVolume() const
{
    return ClampVolume(InterfaceVolume);
}

void UEchoesGameUserSettings::SetInterfaceVolume(float NewVolume)
{
    InterfaceVolume = ClampVolume(NewVolume);
}

float UEchoesGameUserSettings::GetAmbienceVolume() const
{
    return ClampVolume(AmbienceVolume);
}

void UEchoesGameUserSettings::SetAmbienceVolume(float NewVolume)
{
    AmbienceVolume = ClampVolume(NewVolume);
}

float UEchoesGameUserSettings::GetAudioCategoryVolume(
    EEchoesAudioCategory Category) const
{
    switch (Category)
    {
        case EEchoesAudioCategory::Music:
            return GetMusicVolume();
        case EEchoesAudioCategory::Dialogue:
            return GetDialogueVolume();
        case EEchoesAudioCategory::Interface:
            return GetInterfaceVolume();
        case EEchoesAudioCategory::Ambience:
            return GetAmbienceVolume();
        case EEchoesAudioCategory::Effects:
        default:
            return GetEffectsVolume();
    }
}

void UEchoesGameUserSettings::SetAudioCategoryVolume(
    EEchoesAudioCategory Category,
    float NewVolume)
{
    switch (Category)
    {
        case EEchoesAudioCategory::Music:
            SetMusicVolume(NewVolume);
            return;
        case EEchoesAudioCategory::Dialogue:
            SetDialogueVolume(NewVolume);
            return;
        case EEchoesAudioCategory::Interface:
            SetInterfaceVolume(NewVolume);
            return;
        case EEchoesAudioCategory::Ambience:
            SetAmbienceVolume(NewVolume);
            return;
        case EEchoesAudioCategory::Effects:
        default:
            SetEffectsVolume(NewVolume);
            return;
    }
}

FEchoesAudioMixVolumes UEchoesGameUserSettings::GetAudioMixVolumes() const
{
    FEchoesAudioMixVolumes Volumes;
    Volumes.Master = GetMasterVolume();
    Volumes.Music = GetMusicVolume();
    Volumes.Dialogue = GetDialogueVolume();
    Volumes.Interface = GetInterfaceVolume();
    Volumes.Ambience = GetAmbienceVolume();
    Volumes.Effects = GetEffectsVolume();
    return Volumes;
}
