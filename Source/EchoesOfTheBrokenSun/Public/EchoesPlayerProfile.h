#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"

class UEchoesGameUserSettings;

/**
 * Local player state that must survive independently of campaign ledgers and
 * tactical checkpoints. All fields are serialized explicitly; native layout
 * is never part of the file format.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesPlayerProfile final
{
    static constexpr uint16 SchemaVersion = 1;
    static constexpr uint16 AllTutorialLessonsMask = 0x03FF;

    uint8 ActiveJourneySlot = 1;
    bool bOnboardingOffered = false;
    bool bTutorialOptOut = false;
    /** Contiguous low bits for the ten curriculum lessons, in authored order. */
    uint16 TutorialVerifiedMask = 0;

    FIntPoint Resolution = FIntPoint(1280, 720);
    EWindowMode::Type WindowMode = EWindowMode::Windowed;
    float HudScale = 1.0f;
    bool bHighContrastHud = false;
    bool bReducedMotion = false;
    bool bReducedFlashing = false;
    bool bEdgePan = true;
    float CameraPanSpeedScale = 1.0f;
    float CameraZoomScale = 1.0f;
    float EffectsVolume = 1.0f;
    bool bReducedDynamicRange = false;
    float MasterVolume = 1.0f;
    float MusicVolume = 1.0f;
    float DialogueVolume = 1.0f;
    float InterfaceVolume = 1.0f;
    float AmbienceVolume = 1.0f;

    /** Copies every currently implemented local presentation setting. */
    void CaptureSettings(const UEchoesGameUserSettings& Settings);

    /** Validates the complete profile before mutating the settings object. */
    bool ApplySettings(
        UEchoesGameUserSettings& Settings,
        FString& OutError) const;

    /** Derives mastery through the curriculum reducer from verified facts. */
    [[nodiscard]] bool IsTutorialMasteryComplete() const;

    friend bool operator==(
        const FEchoesPlayerProfile&,
        const FEchoesPlayerProfile&) = default;
};

/** Versioned, bounded, checksummed, transactional Profile.sav persistence. */
class ECHOESOFTHEBROKENSUN_API FEchoesPlayerProfileStore final
{
public:
    [[nodiscard]] static FString GetDefaultPath();

    /**
     * Loads a valid primary or backup. OutExists is false only when neither
     * generation exists. Missing or invalid data never mutates OutProfile.
     */
    static bool LoadWithBackup(
        const FString& Path,
        FEchoesPlayerProfile& OutProfile,
        bool& OutExists,
        FString& OutFeedback);

    /** Writes and validates a temporary generation before rotating a primary. */
    static bool SaveAtomic(
        const FString& Path,
        const FEchoesPlayerProfile& Profile,
        FString& OutFeedback);

    /**
     * Replaces a wholly invalid generation set after explicit player
     * confirmation. Prior bytes, including uncommitted temporary data, are
     * archived and restored exactly if the commit fails. A valid primary or
     * backup refuses reset; a temporary record is never treated as committed.
     */
    static bool ResetPreservingInvalidGenerations(
        const FString& Path,
        const FEchoesPlayerProfile& Defaults,
        FString& OutFeedback);

#if WITH_DEV_AUTOMATION_TESTS
    /** One-shot fault injection after rotation and before primary commit. */
    static void FailNextCommitForTesting();
#endif
};
