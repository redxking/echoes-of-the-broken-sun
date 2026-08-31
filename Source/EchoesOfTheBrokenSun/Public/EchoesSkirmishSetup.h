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

/** A complete, offline-only skirmish deployment request. */
struct ECHOESOFTHEBROKENSUN_API FEchoesSkirmishSetup final
{
    echoes::sim::Faction LocalFaction =
        echoes::sim::Faction::MeridianCompact;
    echoes::sim::Faction OpponentFaction =
        echoes::sim::Faction::KharuunAssemblies;
    EEchoesSkirmishMapPreset MapPreset =
        EEchoesSkirmishMapPreset::GlassScar;
    echoes::sim::AiPersonality AiPersonality =
        echoes::sim::AiPersonality::Adaptive;
    EEchoesSkirmishResourceLevel ResourceLevel =
        EEchoesSkirmishResourceLevel::Standard;

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
    [[nodiscard]] static echoes::sim::ResourcePool StartingResources(
        EEchoesSkirmishResourceLevel Level);

    [[nodiscard]] static FEchoesSkirmishSetup WithNextFaction(
        const FEchoesSkirmishSetup& Setup,
        bool bLocal,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextMap(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextAi(
        const FEchoesSkirmishSetup& Setup,
        int32 Direction);
    [[nodiscard]] static FEchoesSkirmishSetup WithNextResources(
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
