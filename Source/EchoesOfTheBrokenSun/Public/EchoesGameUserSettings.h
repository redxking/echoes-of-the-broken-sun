#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"

#include "EchoesAudioMix.h"

#include "EchoesGameUserSettings.generated.h"

/** Persisted presentation and accessibility controls owned by the local player. */
UCLASS(Config = GameUserSettings, NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesGameUserSettings final
    : public UGameUserSettings
{
    GENERATED_BODY()

public:
    static UEchoesGameUserSettings* Get();

    virtual void SetToDefaults() override;
    virtual void ValidateSettings() override;

    [[nodiscard]] float GetHudScale() const;
    void SetHudScale(float NewScale);
    [[nodiscard]] bool IsHighContrastHudEnabled() const;
    void SetHighContrastHudEnabled(bool bEnabled);
    [[nodiscard]] bool IsReducedMotionEnabled() const;
    void SetReducedMotionEnabled(bool bEnabled);
    [[nodiscard]] bool IsReducedFlashingEnabled() const;
    void SetReducedFlashingEnabled(bool bEnabled);
    [[nodiscard]] bool IsEdgePanEnabled() const;
    void SetEdgePanEnabled(bool bEnabled);
    [[nodiscard]] float GetCameraPanSpeedScale() const;
    void SetCameraPanSpeedScale(float NewScale);
    [[nodiscard]] float GetCameraZoomScale() const;
    void SetCameraZoomScale(float NewScale);
    [[nodiscard]] float GetEffectsVolume() const;
    void SetEffectsVolume(float NewVolume);
    [[nodiscard]] bool IsReducedDynamicRangeEnabled() const;
    void SetReducedDynamicRangeEnabled(bool bEnabled);

    [[nodiscard]] float GetMasterVolume() const;
    void SetMasterVolume(float NewVolume);
    [[nodiscard]] float GetMusicVolume() const;
    void SetMusicVolume(float NewVolume);
    [[nodiscard]] float GetDialogueVolume() const;
    void SetDialogueVolume(float NewVolume);
    [[nodiscard]] float GetInterfaceVolume() const;
    void SetInterfaceVolume(float NewVolume);
    [[nodiscard]] float GetAmbienceVolume() const;
    void SetAmbienceVolume(float NewVolume);

    /** Reads one category's player-facing volume by category. */
    [[nodiscard]] float GetAudioCategoryVolume(
        EEchoesAudioCategory Category) const;
    /** Writes one category's player-facing volume by category. */
    void SetAudioCategoryVolume(
        EEchoesAudioCategory Category,
        float NewVolume);
    /** The complete master-plus-five-category volume set. */
    [[nodiscard]] FEchoesAudioMixVolumes GetAudioMixVolumes() const;

private:
    UPROPERTY(Config)
    float HudScale = 1.0f;

    UPROPERTY(Config)
    bool bHighContrastHud = false;

    UPROPERTY(Config)
    bool bReducedMotion = false;

    UPROPERTY(Config)
    bool bReducedFlashing = false;

    UPROPERTY(Config)
    bool bEdgePan = true;

    UPROPERTY(Config)
    float CameraPanSpeedScale = 1.0f;

    UPROPERTY(Config)
    float CameraZoomScale = 1.0f;

    UPROPERTY(Config)
    float EffectsVolume = 1.0f;

    UPROPERTY(Config)
    bool bReducedDynamicRange = false;

    UPROPERTY(Config)
    float MasterVolume = 1.0f;

    UPROPERTY(Config)
    float MusicVolume = 1.0f;

    UPROPERTY(Config)
    float DialogueVolume = 1.0f;

    UPROPERTY(Config)
    float InterfaceVolume = 1.0f;

    UPROPERTY(Config)
    float AmbienceVolume = 1.0f;
};
