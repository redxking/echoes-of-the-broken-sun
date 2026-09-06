#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesSkirmishMapPreset : uint8
{
    GlassScar,
    CrownfallBasin,
    SorynConfluence
};

enum class EEchoesSkirmishResourceLevel : uint8
{
    Scarce,
    Standard,
    Abundant
};

enum class EEchoesSkirmishDifficulty : uint8
{
    Story = 0,
    Standard = 1,
    Veteran = 2,
    Sovereign = 3,
    // Preserve source and serialized ordinal compatibility for prior checkpoints.
    Assisted = Story,
    Challenging = Veteran
};

enum class EEchoesSkirmishVictoryCondition : uint8
{
    Corefall,
    // Legacy ordinals remain readable but validation refuses unsupported rules.
    WellControl,
    Conquest
};

enum class EEchoesSkirmishGameSpeed : uint8
{
    Tactical,
    Normal,
    Fast
};

enum class EEchoesSkirmishTeamSetup : uint8
{
    OneVsOne,
    FreeForAll
};

/** Owner-selected SPEC-DIF policy; all tiers retain identical combat/economic rules. */
struct FEchoesAiDifficultyPolicy final
{
    uint64 ReactionTicks = 30;
    uint64 StrategicReviewTicks = 100;
    int32 GroupCommandsPerSecond = 7;
    static FEchoesAiDifficultyPolicy For(EEchoesSkirmishDifficulty Difficulty);
};

/** A complete, offline-only skirmish deployment request. */
struct ECHOESOFTHEBROKENSUN_API FEchoesSkirmishSetup final
{
    /** Restart retains this seed; Rematch supplies a new nonzero seed. */
    uint64 Seed = 0xE0C0B5A1ULL;
    echoes::sim::Faction LocalFaction =
        echoes::sim::Faction::MeridianCompact;
    echoes::sim::Faction OpponentFaction =
        echoes::sim::Faction::KharuunAssemblies;
    EEchoesSkirmishTeamSetup TeamSetup =
        EEchoesSkirmishTeamSetup::OneVsOne;
    EEchoesSkirmishMapPreset MapPreset =
        EEchoesSkirmishMapPreset::GlassScar;
    echoes::sim::AiPersonality AiPersonality =
        echoes::sim::AiPersonality::Adaptive;
    EEchoesSkirmishDifficulty Difficulty =
        EEchoesSkirmishDifficulty::Standard;
    EEchoesSkirmishResourceLevel ResourceLevel =
        EEchoesSkirmishResourceLevel::Standard;
    EEchoesSkirmishVictoryCondition VictoryCondition =
        EEchoesSkirmishVictoryCondition::Corefall;
    EEchoesSkirmishGameSpeed GameSpeed =
        EEchoesSkirmishGameSpeed::Normal;

    friend bool operator==(
        const FEchoesSkirmishSetup&,
        const FEchoesSkirmishSetup&) = default;
};

/** Stable data and validation shared by setup UI, runtime, and automation. */
struct ECHOESOFTHEBROKENSUN_API FEchoesSkirmishSetupModel final
{
    static constexpr int32 MapWidthTiles = 64;
    static constexpr int32 MapHeightTiles = 64;

    [[nodiscard]] static FEchoesSkirmishSetup DefaultSetup();
    /** Immutable ruleset used by the current direct-connect Online 1v1 path. */
    [[nodiscard]] static FEchoesSkirmishSetup CanonicalOnlineSetup();
    [[nodiscard]] static bool IsCanonicalOnlineSetup(
        const FEchoesSkirmishSetup& Setup);
    [[nodiscard]] static bool Validate(
        const FEchoesSkirmishSetup& Setup,
        FString& OutError);

    [[nodiscard]] static const TCHAR* FactionDisplayName(
        echoes::sim::Faction Faction);
    [[nodiscard]] static const TCHAR* MapDisplayName(
        EEchoesSkirmishMapPreset Preset);
    [[nodiscard]] static const TCHAR* MapDescription(
        EEchoesSkirmishMapPreset Preset);
    [[nodiscard]] static const TCHAR* AiDisplayName(
        echoes::sim::AiPersonality Personality);
    [[nodiscard]] static const TCHAR* AiDescription(
        echoes::sim::AiPersonality Personality);
    [[nodiscard]] static const TCHAR* ResourceDisplayName(
        EEchoesSkirmishResourceLevel Level);
    [[nodiscard]] static const TCHAR* ResourceDescription(
        EEchoesSkirmishResourceLevel Level);
    [[nodiscard]] static echoes::sim::ResourcePool StartingResources(
        EEchoesSkirmishResourceLevel Level);
    [[nodiscard]] static const TCHAR* DifficultyDisplayName(
        EEchoesSkirmishDifficulty Difficulty);
    [[nodiscard]] static const TCHAR* DifficultyDescription(
        EEchoesSkirmishDifficulty Difficulty);
    [[nodiscard]] static const TCHAR* AssistedDifficultyModifiers();
    [[nodiscard]] static const TCHAR* VictoryConditionDisplayName(
        EEchoesSkirmishVictoryCondition Condition);
    [[nodiscard]] static const TCHAR* VictoryConditionDescription(
        EEchoesSkirmishVictoryCondition Condition);
    [[nodiscard]] static const TCHAR* GameSpeedDisplayName(
        EEchoesSkirmishGameSpeed Speed);
    [[nodiscard]] static const TCHAR* GameSpeedDescription(
        EEchoesSkirmishGameSpeed Speed);
    [[nodiscard]] static float GameSpeedMultiplier(
        EEchoesSkirmishGameSpeed Speed);
    [[nodiscard]] static const TCHAR* TeamSetupDisplayName(
        EEchoesSkirmishTeamSetup TeamSetup);
    [[nodiscard]] static const TCHAR* TeamSetupDescription(
        EEchoesSkirmishTeamSetup TeamSetup);

    [[nodiscard]] static FEchoesSkirmishSetup WithNextFaction(
        const FEchoesSkirmishSetup& Setup,
        bool bLocal,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextTeam(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextMap(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextAi(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextDifficulty(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextResources(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextVictoryCondition(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextGameSpeed(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);

    /** Preset terrain predicate. Every preset retains the proven 64x64 contract. */
    [[nodiscard]] static bool IsBlockedTile(
        EEchoesSkirmishMapPreset Preset,
        int32 TileX,
        int32 TileY);
    [[nodiscard]] static int32 ExpectedBlockedTileCount(
        EEchoesSkirmishMapPreset Preset);
    [[nodiscard]] static FIntPoint FutureWellTile(
        EEchoesSkirmishMapPreset Preset);
    [[nodiscard]] static TArray<FIntPoint> LocalSpawnTiles(
        EEchoesSkirmishMapPreset Preset);
    [[nodiscard]] static TArray<FIntPoint> OpponentSpawnTiles(
        EEchoesSkirmishMapPreset Preset);
    [[nodiscard]] static TArray<FIntPoint> ResourceNodeTiles(
        EEchoesSkirmishMapPreset Preset);
};
