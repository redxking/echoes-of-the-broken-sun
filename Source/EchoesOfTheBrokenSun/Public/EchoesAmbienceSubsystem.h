#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "EchoesAmbienceSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/** The authored ambience bed for the site the local presentation shows. */
UENUM()
enum class EEchoesAmbienceBed : uint8
{
    /** No ambience. The initial state and the state after teardown. */
    None,
    /** Glass Scar basin: wind across vitrified glass. */
    GlassScar,
    /** Lume Reach settlement. */
    LumeReach,
    /** Ark-city reserve grid districts. */
    ArkCity,
    /** Crownfall index. */
    Crownfall
};

/**
 * Owns the non-positional site ambience bed and the Future Well proximity
 * layer for the local world.
 *
 * Beds crossfade on site change. The Well layer is additive and follows the
 * presentation's notion of Well proximity. Presentation only: nothing here
 * enters simulation state, fog authority, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesAmbienceSubsystem final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Selects the site bed. Crossfades; never a hard cut. */
    void SetAmbienceBed(EEchoesAmbienceBed Bed);

    /** Raises or lowers the additive Future Well proximity layer. */
    void SetWellProximity(bool bNearWell);

    [[nodiscard]] EEchoesAmbienceBed GetAmbienceBed() const
    {
        return CurrentBed;
    }
    [[nodiscard]] bool IsWellLayerActive() const { return bWellActive; }

    /** True when all five registered ambience cues resolved to SoundWaves. */
    [[nodiscard]] bool HasAllAuthoredCues() const;
    [[nodiscard]] int32 GetLoadedCueCount() const;

    /** True when every loaded cue routes to the ambience category submix. */
    [[nodiscard]] bool HasAmbienceSubmixRouting() const;

    [[nodiscard]] static constexpr float GetCrossfadeSeconds()
    {
        return 2.4f;
    }

    /** The bed cue a site resolves to; null for None or a missing cue. */
    [[nodiscard]] USoundBase* ResolveBedCue(EEchoesAmbienceBed Bed) const;

#if WITH_DEV_AUTOMATION_TESTS
    [[nodiscard]] UAudioComponent* GetBedComponentForTest() const
    {
        return BedComponent;
    }
    [[nodiscard]] UAudioComponent* GetWellComponentForTest() const
    {
        return WellComponent;
    }
#endif

private:
    void StopAllAmbience();
    [[nodiscard]] USoundBase* LoadCue(const TCHAR* Path);

    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<USoundBase>> LoadedCues;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> BedComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> WellComponent;

    EEchoesAmbienceBed CurrentBed = EEchoesAmbienceBed::None;
    bool bWellActive = false;
};
