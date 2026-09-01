#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "EchoesInterfaceAudioSubsystem.generated.h"

class USoundBase;

/** Short interface feedback cues. */
UENUM()
enum class EEchoesInterfaceCue : uint8
{
    Hover,
    Select,
    Confirm,
    Reject,
    MenuOpen,
    MenuClose,
    BriefAdvance
};

/** Rate-limited player alerts. */
UENUM()
enum class EEchoesAlertCue : uint8
{
    UnderAttack,
    StructureLost,
    ProductionComplete,
    ResearchComplete,
    CapacityLow
};

/**
 * Owns interface feedback and alert playback for the local world.
 *
 * Interface cues carry a short per-cue admission window so key repeat and
 * pointer sweeps cannot stack voices. Alerts carry a longer per-class window
 * so a simultaneous-event burst produces one brief cue per class rather than
 * a siren. All twelve cues route to the interface category submix, and the
 * player's interface volume owns their output.
 *
 * Presentation only: nothing here enters command validation, simulation
 * timing, visibility, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesInterfaceAudioSubsystem final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Plays one interface cue; false when rate-limited, muted, or absent. */
    bool PlayInterfaceCue(EEchoesInterfaceCue Cue);

    /** Plays one alert; false when rate-limited, muted, or absent. */
    bool PlayAlert(EEchoesAlertCue Alert);

    /** True when all twelve registered cues resolved to SoundWaves. */
    [[nodiscard]] bool HasAllAuthoredCues() const;
    [[nodiscard]] int32 GetLoadedCueCount() const;

    /** True when every loaded cue routes to the interface category submix. */
    [[nodiscard]] bool HasInterfaceSubmixRouting() const;

    /** Admission window between repeats of one interface cue. */
    [[nodiscard]] static constexpr float GetInterfaceCooldownSeconds()
    {
        return 0.06f;
    }
    /** Admission window between repeats of one alert class. */
    [[nodiscard]] static constexpr float GetAlertCooldownSeconds()
    {
        return 4.0f;
    }

    [[nodiscard]] USoundBase* ResolveInterfaceCue(
        EEchoesInterfaceCue Cue) const;
    [[nodiscard]] USoundBase* ResolveAlertCue(EEchoesAlertCue Alert) const;

#if WITH_DEV_AUTOMATION_TESTS
    bool ReserveInterfaceCueForTest(EEchoesInterfaceCue Cue, double Seconds);
    bool ReserveAlertForTest(EEchoesAlertCue Alert, double Seconds);
    void ResetRateLimitsForTest();
    [[nodiscard]] int32 GetInterfacePlayCountForTest() const
    {
        return InterfacePlayCount;
    }
    [[nodiscard]] int32 GetAlertPlayCountForTest() const
    {
        return AlertPlayCount;
    }
#endif

private:
    [[nodiscard]] bool ReserveInterfaceCue(
        EEchoesInterfaceCue Cue,
        double Seconds);
    [[nodiscard]] bool ReserveAlert(EEchoesAlertCue Alert, double Seconds);
    [[nodiscard]] bool IsInterfaceAudible() const;
    [[nodiscard]] USoundBase* LoadCue(const TCHAR* Path);

    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<USoundBase>> LoadedCues;

    double LastInterfaceSeconds[7] = {
        -1000.0, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0};
    double LastAlertSeconds[5] = {
        -1000.0, -1000.0, -1000.0, -1000.0, -1000.0};
    int32 InterfacePlayCount = 0;
    int32 AlertPlayCount = 0;
};
