#pragma once

#include "CoreMinimal.h"
#include "EchoesPrologueMissionModel.h"
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
        std::optional<echoes::sim::PlayerId> ScopedPlayer = std::nullopt,
        std::optional<EEchoesOperationMode> OperationMode = std::nullopt);
    bool SyncTerrain(const echoes::sim::Simulation& Simulation);
    bool InitializeScopedTerrain(
        int32 InMapWidthTiles,
        int32 InMapHeightTiles,
        float TileWorldSize,
        EEchoesSkirmishMapPreset MapPreset =
            EEchoesSkirmishMapPreset::GlassScar,
        std::optional<EEchoesOperationMode> OperationMode = std::nullopt);
    bool SyncScopedTerrain(
        const std::vector<echoes::sim::net::ScopedTileState>& Tiles);

    // Glass Scar dressing (map_dressing_v1): digest-pinned shelf and shard
    // records consumed as two more presentation-only instanced layers. A
    // record is placed only where the live simulation still reports its cell
    // Blocked, drawn only where the local information state is not
    // Unexplored, and hidden while a Reshape Well holds that cell open, so
    // dressing can never imply an affordance the simulation does not grant.
    [[nodiscard]] bool IsDressingActive() const { return bDressingActive; }
    [[nodiscard]] int32 GetDressingRecordCount() const
    {
        return DressingRecordCount;
    }
    // Records whose cell the live terrain reports Blocked (conformant).
    [[nodiscard]] int32 GetDressingPlacedCount() const
    {
        return DressingPlacedCount;
    }
    // Records refused at runtime because their cell is no longer Blocked.
    [[nodiscard]] int32 GetDressingRefusedCount() const
    {
        return DressingRefusedCount;
    }
    // Records currently drawn: conformant, explored, and not reshaped open.
    [[nodiscard]] int32 GetDressingInstancedCount() const
    {
        return DressingInstancedCount;
    }
    [[nodiscard]] bool IsDressingRecordInstanced(int32 RecordIndex) const;
    [[nodiscard]] bool AreDressingLayersPresentationOnly() const;
    [[nodiscard]] const TCHAR* GetActiveDressingSiteId() const
    {
        return *ActiveDressingSiteId;
    }
    // The player whose information state gates this view, if any. Unset means
    // the legacy full-disclosure path, on which every authored tile is drawn.
    [[nodiscard]] std::optional<echoes::sim::PlayerId> GetScopedPlayer() const
    {
        return ScopedPlayerId;
    }

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

    // Dressing consumer. Initialize adds one hidden instance per record on
    // the class's layer; Sync re-evaluates every record against the terrain,
    // information-state, and reshape sources the caller supplies.
    bool InitializeDressing();
    void SyncDressingWith(
        TFunctionRef<echoes::sim::Terrain(int32, int32)> TerrainAt,
        TFunctionRef<echoes::sim::Visibility(int32, int32)> VisibilityAt,
        TFunctionRef<bool(int32, int32)> ReshapedOpenAt);
    [[nodiscard]] FTransform DressingTransform(int32 RecordIndex) const;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> BlockedTiles;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> ScarredTiles;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> DressingShelves;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> DressingShards;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> BlockedMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> ScarredMaterials;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> BlockedMesh;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> ScarredMesh;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> ShardMesh;

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

    bool bDressingActive = false;
    // True between Initialize and the first Sync, which proves the live
    // terrain is the bound compiled pack before anything draws.
    bool bDressingAwaitingIdentity = false;
    int32 DressingRecordCount = 0;
    int32 DressingPlacedCount = 0;
    int32 DressingRefusedCount = 0;
    int32 DressingInstancedCount = 0;
    // Per-record instance index on its class layer, and the last drawn state
    // (255 never written; otherwise 0 hidden / 1 drawn).
    TArray<int32> DressingInstanceIndex;
    TArray<uint8> DressingDrawnState;
    TArray<bool> DressingRefusalReported;

    struct FActiveDressingRecord
    {
        bool bIsShardLayer = false;
        uint8 X = 0;
        uint8 Y = 0;
        uint8 OrientationOrdinal = 0;
        uint8 ScaleBand = 0;
        int32 CellIndex = 0;
        const char* Id = nullptr;
    };

    enum class EDressingSiteProfile : uint8
    {
        None,
        GlassScar,
        LumeReach
    };

    EDressingSiteProfile ActiveDressingProfile = EDressingSiteProfile::None;
    FString ActiveDressingSiteId;
    FString ActiveDressingPackSha;
    FString ActiveDressingBasePackSha;
    TArray<FActiveDressingRecord> ActiveDressingRecords;
    std::optional<EEchoesOperationMode> ActiveOperationMode;
};
