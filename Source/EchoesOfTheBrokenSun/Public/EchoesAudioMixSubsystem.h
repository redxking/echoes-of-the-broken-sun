#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "EchoesAudioMix.h"

#include "EchoesAudioMixSubsystem.generated.h"

class USoundSubmix;

/**
 * Owns the presentation mix graph: one master submix with five category
 * children — music, dialogue, interface, ambience, and effects.
 *
 * The subsystem is the authority on the linear gain each category carries. It
 * resolves that gain from the player's own volume controls and the reduced
 * dynamic range setting, applies it to the category's submix, and retains the
 * applied value so a headless test can read exactly what the graph carries.
 *
 * The graph is presentation only. It never feeds command validation,
 * simulation timing, visibility, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesAudioMixSubsystem final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Re-reads the player's settings and reapplies every category gain. */
    void ApplyPlayerVolumes();

    /** Applies an explicit volume set. Used by settings changes and by tests. */
    void ApplyVolumes(
        const FEchoesAudioMixVolumes& Volumes,
        bool bReducedDynamicRange);

    /** The linear gain the graph currently carries for one category. */
    [[nodiscard]] float GetAppliedCategoryGain(
        EEchoesAudioCategory Category) const;

    /** The submix a category's cues route to, or null if the graph is absent. */
    [[nodiscard]] USoundSubmix* GetCategorySubmix(
        EEchoesAudioCategory Category) const;

    /** The master submix every category child feeds. */
    [[nodiscard]] USoundSubmix* GetMasterSubmix() const
    {
        return MasterSubmix.Get();
    }

    /** True when the master and all five category submixes exist. */
    [[nodiscard]] bool HasCompleteGraph() const;

    /** True when every category submix is a child of the master submix. */
    [[nodiscard]] bool HasMasterRouting() const;

    /** The volume set most recently applied to the graph. */
    [[nodiscard]] const FEchoesAudioMixVolumes& GetAppliedVolumes() const
    {
        return AppliedVolumes;
    }

    /** Whether the most recent application ran with reduced dynamic range. */
    [[nodiscard]] bool WasAppliedWithReducedDynamicRange() const
    {
        return bAppliedReducedDynamicRange;
    }

    /** Largest minus smallest applied gain across the five categories. */
    [[nodiscard]] float GetAppliedGainSpread() const;

private:
    void BuildGraph();

    UPROPERTY(Transient)
    TObjectPtr<USoundSubmix> MasterSubmix;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USoundSubmix>> CategorySubmixes;

    UPROPERTY(Transient)
    FEchoesAudioMixVolumes AppliedVolumes;

    float AppliedGains[EchoesAudioCategoryCount] = {};
    bool bAppliedReducedDynamicRange = false;
};
