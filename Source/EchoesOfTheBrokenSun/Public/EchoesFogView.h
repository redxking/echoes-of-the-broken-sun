#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesFogView.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

/**
 * Disposable presentation of the local player's authoritative visibility grid.
 * One instanced layer is opaque unexplored fog; the second is the dim explored
 * shroud. Currently visible tiles hide both layers.
 *
 * The unexplored layer is a volume tall enough to occlude the terrain view's
 * tile silhouettes, not a ground-level tint, so an unscouted cliff cannot be
 * read over the top of it. The explored layer stays a thin slab because the
 * information-state table lets a player keep remembered terrain.
 */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesFogView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesFogView();

    bool InitializeFog(
        const echoes::sim::Simulation& Simulation,
        echoes::sim::PlayerId Player,
        float TileWorldSize);
    bool SyncVisibility(const echoes::sim::Simulation& Simulation);
    bool InitializeScopedFog(
        int32 InMapWidthTiles,
        int32 InMapHeightTiles,
        float TileWorldSize);
    bool SyncScopedVisibility(
        const std::vector<echoes::sim::net::ScopedTileState>& Tiles);

    [[nodiscard]] int32 GetUnexploredTileCount() const
    {
        return UnexploredTileCount;
    }
    [[nodiscard]] int32 GetExploredTileCount() const
    {
        return ExploredTileCount;
    }
    [[nodiscard]] int32 GetVisibleTileCount() const
    {
        return VisibleTileCount;
    }
    [[nodiscard]] int32 GetKnownTileCount() const
    {
        return ExploredTileCount + VisibleTileCount;
    }

    void UpdateAccessibilitySettings(bool bInReducedMotion, bool bInReducedFlashing);

    [[nodiscard]] bool IsReducedMotionApplied() const { return bReducedMotion; }
    [[nodiscard]] bool IsReducedFlashingApplied() const { return bReducedFlashing; }
    [[nodiscard]] FLinearColor GetUnexploredBaseColor() const { return UnexploredBaseColor; }
    [[nodiscard]] FLinearColor GetUnexploredBleedColor() const { return UnexploredBleedColor; }
    [[nodiscard]] float GetUnexploredBleedStrength() const { return UnexploredBleedStrength; }
    [[nodiscard]] FLinearColor GetExploredColor() const { return ExploredColor; }

    [[nodiscard]] bool HasCollisionDisabled() const;
    [[nodiscard]] bool HasNavigationDisabled() const;
    [[nodiscard]] bool HasShadowsDisabled() const;
    [[nodiscard]] bool HasOverlapsDisabled() const;

    [[nodiscard]] double GetLastSyncDurationMs() const { return LastSyncDurationMs; }
    [[nodiscard]] uint64 GetTotalSyncCount() const { return TotalSyncCount; }

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FEchoesProductionFogTest;
#endif
    void ApplyMaterials();
    // bUnexplored selects the tall occluding volume; otherwise the thin
    // explored dimming slab.
    [[nodiscard]] FTransform TileTransform(
        int32 TileX,
        int32 TileY,
        bool bUnexplored) const;
    [[nodiscard]] static FTransform HiddenTransform();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> UnexploredTiles;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> ExploredTiles;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> UnexploredMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ExploredMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> BasicMaterial;

    TArray<uint8> CachedVisibility;
    echoes::sim::PlayerId PlayerId = 0;
    int32 MapWidthTiles = 0;
    int32 MapHeightTiles = 0;
    float WorldUnitsPerTile = 200.0f;
    int32 UnexploredTileCount = 0;
    int32 ExploredTileCount = 0;
    int32 VisibleTileCount = 0;

    bool bReducedMotion = false;
    bool bReducedFlashing = false;
    FLinearColor UnexploredBaseColor = FLinearColor(0.004f, 0.003f, 0.005f, 1.0f);
    FLinearColor UnexploredBleedColor = FLinearColor(0.06f, 0.012f, 0.08f, 1.0f);
    float UnexploredBleedStrength = 0.35f;
    FLinearColor ExploredColor = FLinearColor(0.015f, 0.022f, 0.035f, 0.72f);
    double LastSyncDurationMs = 0.0;
    uint64 TotalSyncCount = 0;
};
