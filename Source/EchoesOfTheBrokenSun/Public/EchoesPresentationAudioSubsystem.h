#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesPresentationAudioSubsystem.generated.h"

class USoundBase;
class USoundAttenuation;

UENUM()
enum class EEchoesPresentationAudioCue : uint8
{
    CommandConfirm,
    DestructionMeridian,
    DestructionKharuun
};

/**
 * Owns short, rate-limited presentation sounds for the local world.
 *
 * The subsystem consumes accepted presentation events only. It never feeds
 * command validation, simulation timing, visibility, saves, replays, or hashes.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesPresentationAudioSubsystem final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    bool PlayCommandConfirmation();
    bool PlayDestruction(echoes::sim::Faction Faction, const FVector& Location);

    [[nodiscard]] bool HasAllAuthoredCueAssets() const;
    [[nodiscard]] int32 GetLoadedCueCount() const;
    [[nodiscard]] bool HasBoundedSpatialAttenuation() const;
    [[nodiscard]] static constexpr float GetCommandCooldownSeconds()
    {
        return 0.08f;
    }
    [[nodiscard]] static constexpr float GetDestructionCooldownSeconds()
    {
        return 0.14f;
    }

#if WITH_DEV_AUTOMATION_TESTS
    bool ReserveCueForTest(
        EEchoesPresentationAudioCue Cue,
        double TimeSeconds,
        float EffectsVolume);
    void ResetRateLimitsForTest();
    [[nodiscard]] float GetCueVolumeForTest(
        EEchoesPresentationAudioCue Cue,
        float EffectsVolume,
        bool bReducedDynamicRange) const;
    [[nodiscard]] int32 GetSuccessfulCommandPlayCountForTest() const
    {
        return SuccessfulCommandPlayCount;
    }
    [[nodiscard]] int32 GetSuccessfulDestructionPlayCountForTest() const
    {
        return SuccessfulDestructionPlayCount;
    }
#endif

private:
    bool ReserveCue(
        EEchoesPresentationAudioCue Cue,
        double TimeSeconds,
        float EffectsVolume);
    [[nodiscard]] float GetCueVolume(
        EEchoesPresentationAudioCue Cue,
        float EffectsVolume,
        bool bReducedDynamicRange) const;
    [[nodiscard]] USoundBase* GetCueAsset(
        EEchoesPresentationAudioCue Cue) const;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> CommandConfirmSound;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> MeridianDestructionSound;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> KharuunDestructionSound;

    UPROPERTY(Transient)
    TObjectPtr<USoundAttenuation> DestructionAttenuation;

    double LastCommandSeconds = -1000.0;
    double LastDestructionSeconds = -1000.0;
    int32 SuccessfulCommandPlayCount = 0;
    int32 SuccessfulDestructionPlayCount = 0;
};
