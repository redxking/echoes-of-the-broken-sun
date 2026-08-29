#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

    [[nodiscard]] int32 GetBlockedTileCount() const { return BlockedTileCount; }
    [[nodiscard]] int32 GetScarredTileCount() const { return ScarredTileCount; }

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
    TObjectPtr<UMaterialInstanceDynamic> BlockedMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ScarredMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> BasicMaterial;

    TArray<uint8> CachedTerrain;
    int32 MapWidthTiles = 0;
    int32 MapHeightTiles = 0;
    float WorldUnitsPerTile = 200.0f;
    int32 BlockedTileCount = 0;
    int32 ScarredTileCount = 0;
};
