#pragma once

#include "CoreMinimal.h"

#include "EchoesAudioMix.generated.h"

/**
 * The five player-facing audio categories the mix graph carries.
 *
 * Every registered cue belongs to exactly one category. The category owns the
 * player-facing volume control and the submix the cue is routed to. Categories
 * are presentation only: none of them enters simulation state, fog authority,
 * saves, replays, or checksums.
 */
UENUM()
enum class EEchoesAudioCategory : uint8
{
    Music,
    Dialogue,
    Interface,
    Ambience,
    Effects
};

/** Number of distinct categories in the mix graph. */
inline constexpr int32 EchoesAudioCategoryCount = 5;

/** Ordered category list, index-aligned with the applied-gain array. */
inline constexpr EEchoesAudioCategory EchoesAudioCategories[EchoesAudioCategoryCount] = {
    EEchoesAudioCategory::Music,
    EEchoesAudioCategory::Dialogue,
    EEchoesAudioCategory::Interface,
    EEchoesAudioCategory::Ambience,
    EEchoesAudioCategory::Effects
};

/** Player-facing volume positions, one master and one per category. */
USTRUCT()
struct FEchoesAudioMixVolumes
{
    GENERATED_BODY()

    UPROPERTY()
    float Master = 1.0f;

    UPROPERTY()
    float Music = 1.0f;

    UPROPERTY()
    float Dialogue = 1.0f;

    UPROPERTY()
    float Interface = 1.0f;

    UPROPERTY()
    float Ambience = 1.0f;

    UPROPERTY()
    float Effects = 1.0f;
};

namespace EchoesAudioMix
{
/**
 * Reference gain that reduced dynamic range compresses every category toward.
 *
 * Compression runs across the whole graph rather than on the effects bus alone,
 * so a player who needs a narrower range gets it for music, dialogue,
 * interface, ambience, and effects together.
 */
inline constexpr float ReducedRangeReferenceGain = 0.62f;

/** Fraction of each category's distance from the reference that survives. */
inline constexpr float ReducedRangeRetainedSpread = 0.55f;

/** Stable, log-safe category name. Never localized; never player-facing. */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API const TCHAR* CategoryStableName(
    EEchoesAudioCategory Category);

/** Index of a category in EchoesAudioCategories. */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API int32 CategoryIndex(
    EEchoesAudioCategory Category);

/** The player's raw volume position for one category, clamped to [0, 1]. */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API float CategoryVolume(
    const FEchoesAudioMixVolumes& Volumes,
    EEchoesAudioCategory Category);

/**
 * Linear output gain the graph applies to one category.
 *
 * The category volume scales the category. The master volume scales the whole
 * graph. Reduced dynamic range compresses the category's own position toward
 * ReducedRangeReferenceGain before the master is applied, which narrows the
 * spread between the loudest and quietest categories without silencing any of
 * them. A category the player has muted stays muted under every setting.
 */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API float ResolveCategoryGain(
    const FEchoesAudioMixVolumes& Volumes,
    EEchoesAudioCategory Category,
    bool bReducedDynamicRange);
}
