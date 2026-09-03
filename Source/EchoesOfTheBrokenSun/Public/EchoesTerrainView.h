#pragma once

#include "CoreMinimal.h"
#include "EchoesSkirmishSetup.h"
#include "GameFramework/Actor.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimCore/Simulation.h"

#include <optional>

#include "EchoesTerrainView.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

/**
 * Disposable, non-colliding presentation of authoritative terrain state.
 * The simulation remains the only movement and placement authority.
 *
 * FOG-001 boundary: a tile silhouette is instanced only where the local
 * player's information state is Explored or Visible. Unexplored tiles instance
 * nothing, so the view cannot draw terrain the player has not scouted. Gating
 * is presentation-only (SIM-002): what the simulation stores, reports, and
 * checksums is untouched, and the authoritative census accessors below still
 * report the terrain the view was handed.
 */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesTerrainView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesTerrainView();

    // ScopedPlayer names the local player whose information state gates what is
    // drawn. Leaving it unset keeps the legacy unscoped behaviour, in which
    // every authored tile is drawn; callers that present to a human player must
    // supply the local player id so unexplored terrain stays undrawn.
    bool InitializeTerrain(
        const echoes::sim::Simulation& Simulation,
        float TileWorldSize,
        EEchoesSkirmishMapPreset MapPreset =
            EEchoesSkirmishMapPreset::GlassScar,
        std::optional<echoes::sim::PlayerId> ScopedPlayer = std::nullopt);
    bool SyncTerrain(const echoes::sim::Simulation& Simulation);
    bool InitializeScopedTerrain(
        int32 InMapWidthTiles,
        int32 InMapHeightTiles,
        float TileWorldSize,
        EEchoesSkirmishMapPreset MapPreset =
            EEchoesSkirmishMapPreset::GlassScar);
    bool SyncScopedTerrain(
        const std::vector<echoes::sim::net::ScopedTileState>& Tiles);

    // Census of the terrain the view was handed, independent of what is drawn.
    // On the scoped-network path an unexplored tile arrives as the protocol's
    // Blocked sentinel, so this count there is (known blocked + unexplored) and
    // says nothing true about hidden terrain; prefer the instanced counts below
    // when the question is what the player can actually see.
    [[nodiscard]] int32 GetBlockedTileCount() const { return BlockedTileCount; }
    [[nodiscard]] int32 GetScarredTileCount() const { return ScarredTileCount; }
    // Tiles whose silhouette is actually instanced, i.e. the terrain the local
    // player's information state authorizes. This is the observable that proves
    // the fog gate: it must stay zero for every unexplored tile.
    [[nodiscard]] int32 GetInstancedBlockedTileCount() const
    {
        return InstancedBlockedTileCount;
    }
    [[nodiscard]] int32 GetInstancedScarredTileCount() const
    {
        return InstancedScarredTileCount;
    }
    [[nodiscard]] EEchoesSkirmishMapPreset GetMapPreset() const
    {
        return ActiveMapPreset;
    }
    [[nodiscard]] bool IsUsingAuthoredTerrainMeshes() const
    {
        return BlockedMesh != nullptr && ScarredMesh != nullptr &&
               AuthoredSurfaceMaterial != nullptr;
    }

private:
    [[nodiscard]] FTransform TileTransform(
        int32 TileX,
        int32 TileY,
        echoes::sim::Terrain Terrain) const;
    [[nodiscard]] static FTransform HiddenTransform();
    // Packs terrain and information state into one cache key so a visibility
    // change re-evaluates the tile even when its terrain is unchanged. Both
    // enums occupy two bits, so 255 stays reserved as the never-written value.
    [[nodiscard]] static uint8 EncodeTileState(
        echoes::sim::Terrain Terrain,
        echoes::sim::Visibility Visibility);
    // Writes one tile's two layer transforms. An unexplored tile resolves to
    // the hidden transform on both layers whatever terrain it reports.
    void ApplyTileState(
        int32 TileIndex,
        int32 TileX,
        int32 TileY,
        echoes::sim::Terrain Terrain,
        echoes::sim::Visibility Visibility);

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> BlockedTiles;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> ScarredTiles;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> BlockedMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> ScarredMaterials;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> BlockedMesh;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> ScarredMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> AuthoredSurfaceMaterial;

    // Packed terrain-and-visibility state per tile; 255 means never written.
    TArray<uint8> CachedTerrain;
    // Unset means the caller has not scoped this view to a player, so every
    // authored tile is drawn. Set means unexplored tiles stay undrawn.
    std::optional<echoes::sim::PlayerId> ScopedPlayerId;
    int32 MapWidthTiles = 0;
    int32 MapHeightTiles = 0;
    float WorldUnitsPerTile = 200.0f;
    EEchoesSkirmishMapPreset ActiveMapPreset =
        EEchoesSkirmishMapPreset::GlassScar;
    int32 BlockedTileCount = 0;
    int32 ScarredTileCount = 0;
    int32 InstancedBlockedTileCount = 0;
    int32 InstancedScarredTileCount = 0;
};
