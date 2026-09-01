#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EchoesSimCore/Simulation.h"

#include "EchoesMusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/** The base musical context the local presentation is in. */
UENUM()
enum class EEchoesMusicContext : uint8
{
    /** No music. The initial state and the state after teardown. */
    Silent,
    /** Title screen and archive. */
    Title,
    /** Skirmish under one faction's theme. */
    FactionTheme,
    /** A campaign act bed. */
    ActBed
};

/** One-shot musical punctuation. */
UENUM()
enum class EEchoesMusicStinger : uint8
{
    Victory,
    Defeat,
    EndingRestoration,
    EndingStabilization,
    EndingExtinguishment,
    EndingOpenEvolution
};

/**
 * Owns music playback for the local world: one base bed chosen by context,
 * two additive threat layers, and one-shot stingers.
 *
 * Context changes crossfade rather than hard-cut. The subsystem consumes
 * presentation-side notifications only; it never reads back into command
 * validation, simulation timing, visibility, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesMusicSubsystem final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Selects the base bed. Faction applies to FactionTheme; Act to ActBed. */
    void SetMusicContext(
        EEchoesMusicContext Context,
        echoes::sim::Faction Faction = echoes::sim::Faction::MeridianCompact,
        int32 ActIndex = 1);

    /** Raises or lowers the additive tension and combat layers. */
    void SetThreatLayers(bool bTension, bool bCombat);

    /** Plays a one-shot stinger over the current bed. */
    bool PlayStinger(EEchoesMusicStinger Stinger);

    [[nodiscard]] EEchoesMusicContext GetMusicContext() const
    {
        return CurrentContext;
    }
    [[nodiscard]] echoes::sim::Faction GetContextFaction() const
    {
        return CurrentFaction;
    }
    [[nodiscard]] int32 GetContextAct() const
    {
        return CurrentAct;
    }
    [[nodiscard]] bool IsTensionLayerActive() const { return bTensionActive; }
    [[nodiscard]] bool IsCombatLayerActive() const { return bCombatActive; }

    /** True when all fifteen registered music cues resolved to SoundWaves. */
    [[nodiscard]] bool HasAllAuthoredCues() const;
    [[nodiscard]] int32 GetLoadedCueCount() const;

    /** True when every loaded cue's base submix is the music category submix. */
    [[nodiscard]] bool HasMusicSubmixRouting() const;

    /** Crossfade window between beds; the no-hard-cut guarantee. */
    [[nodiscard]] static constexpr float GetCrossfadeSeconds()
    {
        return 1.6f;
    }
    /** Fade window for the tension and combat layers. */
    [[nodiscard]] static constexpr float GetLayerFadeSeconds()
    {
        return 0.9f;
    }

    /** The bed cue a context resolves to; null for Silent or a missing cue. */
    [[nodiscard]] USoundBase* ResolveBedCue(
        EEchoesMusicContext Context,
        echoes::sim::Faction Faction,
        int32 ActIndex) const;

    /** The cue a stinger resolves to. */
    [[nodiscard]] USoundBase* ResolveStingerCue(
        EEchoesMusicStinger Stinger) const;

#if WITH_DEV_AUTOMATION_TESTS
    [[nodiscard]] UAudioComponent* GetBedComponentForTest() const
    {
        return BedComponent;
    }
    [[nodiscard]] UAudioComponent* GetTensionComponentForTest() const
    {
        return TensionComponent;
    }
    [[nodiscard]] UAudioComponent* GetCombatComponentForTest() const
    {
        return CombatComponent;
    }
    [[nodiscard]] int32 GetStingerPlayCountForTest() const
    {
        return StingerPlayCount;
    }
#endif

private:
    void StopAllMusic();
    [[nodiscard]] USoundBase* LoadCue(const TCHAR* Path);
    [[nodiscard]] UAudioComponent* StartLoopingBed(
        USoundBase* Sound,
        float FadeSeconds);

    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<USoundBase>> LoadedCues;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> BedComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> TensionComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> CombatComponent;

    EEchoesMusicContext CurrentContext = EEchoesMusicContext::Silent;
    echoes::sim::Faction CurrentFaction =
        echoes::sim::Faction::MeridianCompact;
    int32 CurrentAct = 1;
    bool bTensionActive = false;
    bool bCombatActive = false;
    int32 StingerPlayCount = 0;
};
