#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesTerrainView.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

/**
 * Disposable, non-colliding presentation of authoritative terrain state.
 * The simulation remains the only movement and placement authority.
 */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesTerrainView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesTerrainView();

    bool InitializeTerrain(
        const echoes::sim::Simulation& Simulation,
        float TileWorldSize);
    bool SyncTerrain(const echoes::sim::Simulation& Simulation);
    bool InitializeScopedTerrain(
        int32 InMapWidthTiles,
        int32 InMapHeightTiles,
        float TileWorldSize);
    bool SyncScopedTerrain(
        const std::vector<echoes::sim::net::ScopedTileState>& Tiles);

    [[nodiscard]] int32 GetBlockedTileCount() const { return BlockedTileCount; }
    [[nodiscard]] int32 GetScarredTileCount() const { return ScarredTileCount; }
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

    TArray<uint8> CachedTerrain;
    int32 MapWidthTiles = 0;
    int32 MapHeightTiles = 0;
    float WorldUnitsPerTile = 200.0f;
    int32 BlockedTileCount = 0;
    int32 ScarredTileCount = 0;
};
