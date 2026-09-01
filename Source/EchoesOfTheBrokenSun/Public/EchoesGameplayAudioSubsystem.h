#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EchoesSimCore/Simulation.h"

#include "EchoesGameplayAudioSubsystem.generated.h"

class USoundAttenuation;
class USoundBase;

/**
 * Every authoritative gameplay event the design maps to a sound.
 *
 * This enum is the coverage contract: the automation test walks every value
 * and requires a registered, loadable cue. Adding an event here without a
 * registered cue fails the suite rather than shipping a silent event.
 */
UENUM()
enum class EEchoesGameplayAudioEvent : uint8
{
    WeaponFireLight,
    WeaponFireLine,
    WeaponFireHeavy,
    ImpactHit,
    ImpactShielded,
    GatherMatter,
    DeliverMatter,
    ConstructionStart,
    ConstructionComplete,
    ProductionComplete,
    ResearchStart,
    ResearchInterrupted,
    WellClaim,
    WellHarvest,
    WellPreserve,
    WellReshape,
    ReshapeOpen,
    ReshapeClose,
    Count UMETA(Hidden)
};

/**
 * Owns spatialized gameplay-event playback for the local world.
 *
 * Events arrive from the simulation subsystem's authoritative-state observer,
 * which only reports what the fair-visibility view exposes. Each event class
 * carries an admission window so a 400-unit combat load produces a bounded
 * voice count instead of a wall. Cues route to the effects category submix;
 * the player's effects volume owns their output.
 *
 * Presentation only: nothing here enters command validation, simulation
 * timing, visibility, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesGameplayAudioSubsystem final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Plays one event at a world location; false when limited or muted. */
    bool PlayEvent(
        EEchoesGameplayAudioEvent Event,
        const FVector& WorldLocation);

    /** Non-positional variant for player-scoped events (research). */
    bool PlayEvent2D(EEchoesGameplayAudioEvent Event);

    /** The weapon-fire event for one authoritative attacker archetype. */
    [[nodiscard]] static EEchoesGameplayAudioEvent WeaponEventForType(
        echoes::sim::EntityType Type);

    /** The commitment event for one Future Well protocol choice. */
    [[nodiscard]] static EEchoesGameplayAudioEvent WellEventForChoice(
        echoes::sim::FutureWellChoice Choice);

    [[nodiscard]] bool HasAllAuthoredCues() const;
    [[nodiscard]] int32 GetLoadedCueCount() const;
    [[nodiscard]] bool HasEffectsSubmixRouting() const;
    [[nodiscard]] bool HasBoundedSpatialAttenuation() const;

    /** The registered cue an event resolves to; null only when unloaded. */
    [[nodiscard]] USoundBase* ResolveEventCue(
        EEchoesGameplayAudioEvent Event) const;

    /** Admission window for one event class, in seconds. */
    [[nodiscard]] static float GetEventCooldownSeconds(
        EEchoesGameplayAudioEvent Event);

#if WITH_DEV_AUTOMATION_TESTS
    bool ReserveEventForTest(EEchoesGameplayAudioEvent Event, double Seconds);
    void ResetRateLimitsForTest();
    [[nodiscard]] int32 GetPlayCountForTest() const { return PlayCount; }
#endif

private:
    [[nodiscard]] bool ReserveEvent(
        EEchoesGameplayAudioEvent Event,
        double Seconds);
    [[nodiscard]] bool AreEffectsAudible() const;
    [[nodiscard]] USoundBase* LoadCue(const TCHAR* Path);

    UPROPERTY(Transient)
    TArray<TObjectPtr<USoundBase>> EventCues;

    UPROPERTY(Transient)
    TObjectPtr<USoundAttenuation> SpatialAttenuation;

    double LastEventSeconds[static_cast<int32>(
        EEchoesGameplayAudioEvent::Count)] = {};
    int32 PlayCount = 0;
};
