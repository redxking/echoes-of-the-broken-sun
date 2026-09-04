#include "EchoesTerrainView.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesBattlefieldPresentation.h"
#include "EchoesGlassScarCompiledMapPack.h"
#include "EchoesGlassScarDressingPack.h"
#include "EchoesOfTheBrokenSun.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include <string_view>

namespace
{
const FName TerrainColorParameterName(TEXT("Color"));
const FName TerrainMetallicParameterName(TEXT("Metallic"));
const FName TerrainRoughnessParameterName(TEXT("Roughness"));
const FName TerrainEmissiveParameterName(TEXT("EmissiveStrength"));

namespace dressing = echoes::world::glass_scar_dressing;
namespace map_pack = echoes::world::glass_scar_pack;
// The dressing records were verified against one exact compiled map pack;
// binding them to any other pack is refused at compile time.
static_assert(
    std::string_view(dressing::kBaseCompiledPackSha256) ==
        std::string_view(map_pack::kCompiledPackSha256),
    "Glass Scar dressing records were verified against a different compiled map pack");
static_assert(
    dressing::kGridWidthTiles == map_pack::kGridWidthTiles &&
        dressing::kGridHeightTiles == map_pack::kGridHeightTiles,
    "Glass Scar dressing grid disagrees with the compiled map pack grid");

constexpr float DressingShelfSourceWidth = 780.0f;
constexpr float DressingShelfPlanarBands[3] = {0.82f, 1.0f, 1.18f};
// Shard bands follow the accepted composition-layer convention: authored
// planar scale x1.5, height x0.65.
constexpr float DressingShardPlanarBands[3] = {0.45f, 0.57f, 0.69f};
constexpr float DressingShardHeightBands[3] = {0.40f, 0.51f, 0.62f};
constexpr uint8 DressingNeverDrawn = 255;
}

AEchoesTerrainView::AEchoesTerrainView()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    BlockedTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("BlockedTiles"));
    BlockedTiles->SetupAttachment(SceneRoot);
    ScarredTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("ScarredTiles"));
    ScarredTiles->SetupAttachment(SceneRoot);
    DressingShelves = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("DressingShelves"));
    DressingShelves->SetupAttachment(SceneRoot);
    DressingShards = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("DressingShards"));
    DressingShards->SetupAttachment(SceneRoot);

    for (UInstancedStaticMeshComponent* Layer :
         {BlockedTiles, ScarredTiles, DressingShelves, DressingShards})
    {
        Layer->SetMobility(EComponentMobility::Movable);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCastShadow(false);
        Layer->SetReceivesDecals(false);
        Layer->SetCanEverAffectNavigation(false);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RidgeFinder(
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarRidge.SM_World_GlassScarRidge"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShelfFinder(
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarShelf.SM_World_GlassScarShelf"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShardFinder(
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarShard.SM_World_GlassScarShard"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesWorldSurface.M_EchoesWorldSurface"));
    BlockedMesh = RidgeFinder.Object;
    ScarredMesh = ShelfFinder.Object;
    ShardMesh = ShardFinder.Object;
    AuthoredSurfaceMaterial = MaterialFinder.Object;
    Tags.Add(TEXT("EchoesTerrainView"));
}

FTransform AEchoesTerrainView::TileTransform(
    int32 TileX,
    int32 TileY,
    echoes::sim::Terrain Terrain) const
{
    const float HalfWidth = static_cast<float>(MapWidthTiles) * 0.5f;
    const float HalfHeight = static_cast<float>(MapHeightTiles) * 0.5f;
    const bool bBlocked = Terrain == echoes::sim::Terrain::Blocked;
    const float SourceWidth = bBlocked ? 188.0f : 780.0f;
    const float Inset = bBlocked ? 0.94f : 0.90f;
    const float PlanarScale = WorldUnitsPerTile * Inset / SourceWidth;
    return FTransform(
        FRotator::ZeroRotator,
        FVector(
            (static_cast<float>(TileX) - HalfWidth) * WorldUnitsPerTile,
            (static_cast<float>(TileY) - HalfHeight) * WorldUnitsPerTile,
            bBlocked ? 8.0f : 0.0f),
        bBlocked
            ? FVector(PlanarScale, PlanarScale, 0.62f)
            : FVector(PlanarScale, PlanarScale, PlanarScale * 0.72f));
}

FTransform AEchoesTerrainView::HiddenTransform()
{
    return FTransform(
        FRotator::ZeroRotator,
        FVector::ZeroVector,
        FVector::ZeroVector);
}

uint8 AEchoesTerrainView::EncodeTileState(
    echoes::sim::Terrain Terrain,
    echoes::sim::Visibility Visibility)
{
    return static_cast<uint8>(
        static_cast<uint8>(Terrain) |
        static_cast<uint8>(static_cast<uint8>(Visibility) << 2));
}

void AEchoesTerrainView::ApplyTileState(
    int32 TileIndex,
    int32 TileX,
    int32 TileY,
    echoes::sim::Terrain Terrain,
    echoes::sim::Visibility Visibility)
{
    // The information-state table allows no terrain detail on an unexplored
    // tile, so neither layer may raise a silhouette there. Explored tiles keep
    // their remembered terrain, which the table explicitly permits.
    const bool bKnown = Visibility != echoes::sim::Visibility::Unexplored;
    const FTransform Hidden = HiddenTransform();
    BlockedTiles->UpdateInstanceTransform(
        TileIndex,
        bKnown && Terrain == echoes::sim::Terrain::Blocked
            ? TileTransform(TileX, TileY, Terrain)
            : Hidden,
        false,
        false,
        true);
    ScarredTiles->UpdateInstanceTransform(
        TileIndex,
        bKnown && Terrain == echoes::sim::Terrain::Scarred
            ? TileTransform(TileX, TileY, Terrain)
            : Hidden,
        false,
        false,
        true);
}

bool AEchoesTerrainView::InitializeTerrain(
    const echoes::sim::Simulation& Simulation,
    float TileWorldSize,
    EEchoesSkirmishMapPreset MapPreset,
    std::optional<echoes::sim::PlayerId> ScopedPlayer)
{
    // A named player must exist, otherwise the caller believes it is scoping
    // the view when it is not.
    if (ScopedPlayer.has_value() &&
        Simulation.FindPlayer(*ScopedPlayer) == nullptr)
    {
        return false;
    }
    if (!InitializeScopedTerrain(
            Simulation.Config().mapWidthTiles,
            Simulation.Config().mapHeightTiles,
            TileWorldSize,
            MapPreset))
    {
        return false;
    }
    ScopedPlayerId = ScopedPlayer;
    return SyncTerrain(Simulation);
}

bool AEchoesTerrainView::InitializeScopedTerrain(
    int32 InMapWidthTiles,
    int32 InMapHeightTiles,
    float TileWorldSize,
    EEchoesSkirmishMapPreset MapPreset)
{
    if (MapPreset != EEchoesSkirmishMapPreset::GlassScar &&
        MapPreset != EEchoesSkirmishMapPreset::CrownfallBasin &&
        MapPreset != EEchoesSkirmishMapPreset::SorynConfluence)
    {
        return false;
    }
    ActiveMapPreset = MapPreset;
    if (BlockedMesh == nullptr || ScarredMesh == nullptr ||
        AuthoredSurfaceMaterial == nullptr || InMapWidthTiles <= 0 ||
        InMapHeightTiles <= 0 || InMapWidthTiles > 256 ||
        InMapHeightTiles > 256 || TileWorldSize <= 0.0f)
    {
        return false;
    }
    MapWidthTiles = InMapWidthTiles;
    MapHeightTiles = InMapHeightTiles;
    WorldUnitsPerTile = TileWorldSize;
    // The scoped-network path carries the player's information state in the
    // tile payload itself, so a rebuild starts unscoped and InitializeTerrain
    // re-applies a player only when its caller supplied one.
    ScopedPlayerId.reset();
    BlockedTileCount = 0;
    ScarredTileCount = 0;
    InstancedBlockedTileCount = 0;
    InstancedScarredTileCount = 0;
    const int32 TileCount = MapWidthTiles * MapHeightTiles;
    CachedTerrain.Init(255, TileCount);
    BlockedTiles->SetStaticMesh(BlockedMesh);
    ScarredTiles->SetStaticMesh(ScarredMesh);
    BlockedMaterials.Reset();
    ScarredMaterials.Reset();
    // Slot 3 is the emissive accent. Its hue must stay separated from every
    // owner, faction, resource, and command-marker identity color so terrain
    // never camouflages actors or orders in the composed frame; the
    // PresentationProfiles test enforces the separation floor.
    FLinearColor BlockedColors[] = {
        FLinearColor(0.075f, 0.045f, 0.050f),
        FLinearColor(0.010f, 0.014f, 0.020f),
        FLinearColor(0.24f, 0.18f, 0.15f),
        FLinearColor(0.55f, 0.04f, 0.38f)};
    FLinearColor ScarredColors[] = {
        FLinearColor(0.12f, 0.065f, 0.035f),
        FLinearColor(0.025f, 0.020f, 0.018f),
        FLinearColor(0.30f, 0.21f, 0.13f),
        FLinearColor(0.92f, 0.06f, 0.62f)};
    if (ActiveMapPreset == EEchoesSkirmishMapPreset::CrownfallBasin)
    {
        BlockedColors[0] = FLinearColor(0.055f, 0.090f, 0.105f);
        BlockedColors[1] = FLinearColor(0.012f, 0.025f, 0.030f);
        BlockedColors[2] = FLinearColor(0.26f, 0.22f, 0.12f);
        BlockedColors[3] = FLinearColor(0.48f, 0.78f, 0.09f);
        ScarredColors[0] = FLinearColor(0.070f, 0.11f, 0.085f);
        ScarredColors[1] = FLinearColor(0.018f, 0.030f, 0.025f);
        ScarredColors[2] = FLinearColor(0.31f, 0.24f, 0.11f);
        ScarredColors[3] = FLinearColor(0.62f, 0.95f, 0.18f);
    }
    else if (ActiveMapPreset == EEchoesSkirmishMapPreset::SorynConfluence)
    {
        BlockedColors[0] = FLinearColor(0.075f, 0.050f, 0.13f);
        BlockedColors[1] = FLinearColor(0.012f, 0.018f, 0.035f);
        BlockedColors[2] = FLinearColor(0.10f, 0.28f, 0.31f);
        BlockedColors[3] = FLinearColor(0.15f, 0.25f, 0.95f);
        ScarredColors[0] = FLinearColor(0.045f, 0.10f, 0.15f);
        ScarredColors[1] = FLinearColor(0.015f, 0.022f, 0.040f);
        ScarredColors[2] = FLinearColor(0.18f, 0.24f, 0.42f);
        ScarredColors[3] = FLinearColor(0.30f, 0.42f, 1.0f);
    }
    for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
    {
        UMaterialInstanceDynamic* BlockedMaterial =
            UMaterialInstanceDynamic::Create(AuthoredSurfaceMaterial, this);
        UMaterialInstanceDynamic* ScarredMaterial =
            UMaterialInstanceDynamic::Create(AuthoredSurfaceMaterial, this);
        if (BlockedMaterial == nullptr || ScarredMaterial == nullptr)
        {
            return false;
        }
        const float Emissive = MaterialIndex == 3 ? 1.6f : 0.0f;
        BlockedMaterial->SetVectorParameterValue(
            TerrainColorParameterName,
            BlockedColors[MaterialIndex]);
        ScarredMaterial->SetVectorParameterValue(
            TerrainColorParameterName,
            ScarredColors[MaterialIndex]);
        for (UMaterialInstanceDynamic* Material : {BlockedMaterial, ScarredMaterial})
        {
            Material->SetScalarParameterValue(
                TerrainMetallicParameterName,
                MaterialIndex == 1 ? 0.42f : 0.12f);
            Material->SetScalarParameterValue(
                TerrainRoughnessParameterName,
                MaterialIndex == 1 ? 0.20f : 0.66f);
            Material->SetScalarParameterValue(
                TerrainEmissiveParameterName,
                Emissive);
        }
        BlockedMaterials.Add(BlockedMaterial);
        ScarredMaterials.Add(ScarredMaterial);
        BlockedTiles->SetMaterial(MaterialIndex, BlockedMaterial);
        ScarredTiles->SetMaterial(MaterialIndex, ScarredMaterial);
    }

    Tags.AddUnique(EchoesBattlefieldPresentation::RootTag());
    Tags.AddUnique(EchoesBattlefieldPresentation::TagForPreset(
        ActiveMapPreset));

    BlockedTiles->ClearInstances();
    ScarredTiles->ClearInstances();
    BlockedTiles->PreAllocateInstancesMemory(TileCount);
    ScarredTiles->PreAllocateInstancesMemory(TileCount);
    const FTransform Hidden = HiddenTransform();
    for (int32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
    {
        BlockedTiles->AddInstance(Hidden, false);
        ScarredTiles->AddInstance(Hidden, false);
    }
    return InitializeDressing();
}

bool AEchoesTerrainView::SyncScopedTerrain(
    const std::vector<echoes::sim::net::ScopedTileState>& Tiles)
{
    const int32 TileCount = MapWidthTiles * MapHeightTiles;
    if (Tiles.size() != static_cast<std::size_t>(TileCount) ||
        CachedTerrain.Num() != TileCount ||
        BlockedTiles->GetInstanceCount() != TileCount ||
        ScarredTiles->GetInstanceCount() != TileCount)
    {
        return false;
    }
    BlockedTileCount = 0;
    ScarredTileCount = 0;
    InstancedBlockedTileCount = 0;
    InstancedScarredTileCount = 0;
    bool bChanged = false;
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            const int32 TileIndex = TileY * MapWidthTiles + TileX;
            const echoes::sim::net::ScopedTileState& Tile =
                Tiles[static_cast<std::size_t>(TileIndex)];
            const echoes::sim::Terrain Terrain = Tile.terrain;
            // The scoped protocol forces an unexplored tile to the Blocked
            // sentinel, so its terrain field is an absence of information and
            // not a cliff. Drawing it would invent terrain the player has never
            // scouted, which is why the gate below is on visibility, never on
            // the terrain value.
            const echoes::sim::Visibility Visibility = Tile.visibility;
            const bool bKnown =
                Visibility != echoes::sim::Visibility::Unexplored;
            BlockedTileCount +=
                Terrain == echoes::sim::Terrain::Blocked ? 1 : 0;
            ScarredTileCount +=
                Terrain == echoes::sim::Terrain::Scarred ? 1 : 0;
            InstancedBlockedTileCount +=
                bKnown && Terrain == echoes::sim::Terrain::Blocked ? 1 : 0;
            InstancedScarredTileCount +=
                bKnown && Terrain == echoes::sim::Terrain::Scarred ? 1 : 0;
            const uint8 Encoded = EncodeTileState(Terrain, Visibility);
            if (CachedTerrain[TileIndex] == Encoded)
            {
                continue;
            }
            CachedTerrain[TileIndex] = Encoded;
            ApplyTileState(TileIndex, TileX, TileY, Terrain, Visibility);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        BlockedTiles->MarkRenderStateDirty();
        ScarredTiles->MarkRenderStateDirty();
    }
    SyncDressingWith(
        [&Tiles, this](int32 X, int32 Y)
        {
            return Tiles[static_cast<std::size_t>(Y * MapWidthTiles + X)].terrain;
        },
        [&Tiles, this](int32 X, int32 Y)
        {
            return Tiles[static_cast<std::size_t>(Y * MapWidthTiles + X)]
                .visibility;
        },
        [&Tiles, this](int32 X, int32 Y)
        {
            const echoes::sim::net::ScopedTileState& Tile =
                Tiles[static_cast<std::size_t>(Y * MapWidthTiles + X)];
            return Tile.passable &&
                Tile.terrain == echoes::sim::Terrain::Blocked;
        });
    return true;
}

bool AEchoesTerrainView::SyncTerrain(
    const echoes::sim::Simulation& Simulation)
{
    const int32 TileCount = MapWidthTiles * MapHeightTiles;
    if (Simulation.Config().mapWidthTiles != MapWidthTiles ||
        Simulation.Config().mapHeightTiles != MapHeightTiles ||
        CachedTerrain.Num() != TileCount ||
        BlockedTiles->GetInstanceCount() != TileCount ||
        ScarredTiles->GetInstanceCount() != TileCount)
    {
        return false;
    }

    BlockedTileCount = 0;
    ScarredTileCount = 0;
    InstancedBlockedTileCount = 0;
    InstancedScarredTileCount = 0;
    bool bChanged = false;
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            const int32 TileIndex = TileY * MapWidthTiles + TileX;
            const echoes::sim::Terrain Terrain =
                Simulation.TerrainAt(TileX, TileY);
            // Reading the simulation is only legal here because nothing is
            // drawn from it until the player's own information state allows it.
            // Without a scoped player the caller has claimed full disclosure,
            // which is the legacy behaviour this overload preserves.
            const echoes::sim::Visibility Visibility =
                ScopedPlayerId.has_value()
                    ? Simulation.VisibilityAt(
                          *ScopedPlayerId,
                          echoes::sim::Vec2::FromTiles(TileX, TileY))
                    : echoes::sim::Visibility::Visible;
            const bool bKnown =
                Visibility != echoes::sim::Visibility::Unexplored;
            BlockedTileCount += Terrain == echoes::sim::Terrain::Blocked ? 1 : 0;
            ScarredTileCount += Terrain == echoes::sim::Terrain::Scarred ? 1 : 0;
            InstancedBlockedTileCount +=
                bKnown && Terrain == echoes::sim::Terrain::Blocked ? 1 : 0;
            InstancedScarredTileCount +=
                bKnown && Terrain == echoes::sim::Terrain::Scarred ? 1 : 0;
            const uint8 Encoded = EncodeTileState(Terrain, Visibility);
            if (CachedTerrain[TileIndex] == Encoded)
            {
                continue;
            }
            CachedTerrain[TileIndex] = Encoded;
            ApplyTileState(TileIndex, TileX, TileY, Terrain, Visibility);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        BlockedTiles->MarkRenderStateDirty();
        ScarredTiles->MarkRenderStateDirty();
    }
    SyncDressingWith(
        [&Simulation](int32 X, int32 Y)
        {
            return Simulation.TerrainAt(X, Y);
        },
        [&Simulation, this](int32 X, int32 Y)
        {
            return ScopedPlayerId.has_value()
                ? Simulation.VisibilityAt(
                      *ScopedPlayerId,
                      echoes::sim::Vec2::FromTiles(X, Y))
                : echoes::sim::Visibility::Visible;
        },
        [&Simulation](int32 X, int32 Y)
        {
            // A Blocked tile that reports passable is one a Reshape Well
            // currently holds open; the record hides for that span.
            return Simulation.IsPositionPassable(
                echoes::sim::Vec2::FromTiles(X, Y));
        });
    return true;
}

bool AEchoesTerrainView::InitializeDressing()
{
    bDressingActive = false;
    bDressingAwaitingIdentity = false;
    DressingRecordCount = 0;
    DressingPlacedCount = 0;
    DressingRefusedCount = 0;
    DressingInstancedCount = 0;
    DressingInstanceIndex.Reset();
    DressingDrawnState.Reset();
    DressingRefusalReported.Reset();
    DressingShelves->ClearInstances();
    DressingShards->ClearInstances();

    // Only the Glass Scar vocabulary is populated; the other presets record
    // an empty vocabulary by contract, so an inactive layer is the correct
    // state there rather than a failure.
    if (ActiveMapPreset != EEchoesSkirmishMapPreset::GlassScar ||
        MapWidthTiles != dressing::kGridWidthTiles ||
        MapHeightTiles != dressing::kGridHeightTiles)
    {
        return true;
    }
    if (ShardMesh == nullptr || ScarredMesh == nullptr ||
        BlockedMaterials.Num() != 4)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_DRESSING_REFUSED] site=%s reason=assets shelf=%s shard=%s materials=%d"),
            ANSI_TO_TCHAR(dressing::kSiteId),
            ScarredMesh != nullptr ? TEXT("ready") : TEXT("missing"),
            ShardMesh != nullptr ? TEXT("ready") : TEXT("missing"),
            BlockedMaterials.Num());
        return false;
    }

    DressingShelves->SetStaticMesh(ScarredMesh);
    DressingShards->SetStaticMesh(ShardMesh);
    for (int32 MaterialIndex = 0; MaterialIndex < BlockedMaterials.Num();
         ++MaterialIndex)
    {
        DressingShelves->SetMaterial(MaterialIndex, BlockedMaterials[MaterialIndex]);
        DressingShards->SetMaterial(MaterialIndex, BlockedMaterials[MaterialIndex]);
    }

    const FTransform Hidden = HiddenTransform();
    DressingRecordCount = dressing::kRecordCount;
    DressingInstanceIndex.Init(INDEX_NONE, DressingRecordCount);
    DressingDrawnState.Init(DressingNeverDrawn, DressingRecordCount);
    DressingRefusalReported.Init(false, DressingRecordCount);
    for (int32 RecordIndex = 0; RecordIndex < DressingRecordCount; ++RecordIndex)
    {
        const dressing::FDressingRecord& Record = dressing::kRecords[RecordIndex];
        UInstancedStaticMeshComponent* Layer =
            Record.Class == dressing::EDressingClass::GlassShard
                ? DressingShards.Get()
                : DressingShelves.Get();
        DressingInstanceIndex[RecordIndex] = Layer->AddInstance(Hidden, false);
    }
    bDressingActive = true;
    bDressingAwaitingIdentity = true;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_DRESSING_READY] site=%s records=%d packSha256=%s baseCompiledPackSha256=%s collision=false shadows=false navigation=false authority=presentation"),
        ANSI_TO_TCHAR(dressing::kSiteId),
        DressingRecordCount,
        ANSI_TO_TCHAR(dressing::kPackSha256),
        ANSI_TO_TCHAR(dressing::kBaseCompiledPackSha256));
    return true;
}

FTransform AEchoesTerrainView::DressingTransform(int32 RecordIndex) const
{
    const dressing::FDressingRecord& Record = dressing::kRecords[RecordIndex];
    const float HalfWidth = static_cast<float>(MapWidthTiles) * 0.5f;
    const float HalfHeight = static_cast<float>(MapHeightTiles) * 0.5f;
    const int32 Band = FMath::Clamp<int32>(Record.ScaleBand, 0, 2);
    const FRotator Rotation(
        0.0f,
        90.0f * static_cast<float>(Record.OrientationOrdinal % 4),
        0.0f);
    const FVector Location(
        (static_cast<float>(Record.X) - HalfWidth) * WorldUnitsPerTile,
        (static_cast<float>(Record.Y) - HalfHeight) * WorldUnitsPerTile,
        Record.Class == dressing::EDressingClass::GlassShard ? 10.0f : 4.0f);
    if (Record.Class == dressing::EDressingClass::GlassShard)
    {
        return FTransform(
            Rotation,
            Location,
            FVector(
                DressingShardPlanarBands[Band],
                DressingShardPlanarBands[Band],
                DressingShardHeightBands[Band]));
    }
    const float PlanarScale =
        WorldUnitsPerTile * 0.90f / DressingShelfSourceWidth *
        DressingShelfPlanarBands[Band];
    return FTransform(
        Rotation,
        Location,
        FVector(PlanarScale, PlanarScale, PlanarScale * 0.72f));
}

void AEchoesTerrainView::SyncDressingWith(
    TFunctionRef<echoes::sim::Terrain(int32, int32)> TerrainAt,
    TFunctionRef<echoes::sim::Visibility(int32, int32)> VisibilityAt,
    TFunctionRef<bool(int32, int32)> ReshapedOpenAt)
{
    if (!bDressingActive)
    {
        return;
    }
    // The presentation preset names a theme, not a map. Campaign operations
    // present under the Glass Scar theme on their own terrain, so the first
    // sync is where the live terrain proves it is the compiled pack these
    // records were verified against: any record off a Blocked cell means it
    // is not, and the layer deactivates for this view rather than drawing a
    // partial match or reporting a defect. After activation a refusal is an
    // anomaly and is reported once per record.
    if (bDressingAwaitingIdentity)
    {
        bDressingAwaitingIdentity = false;
        int32 Mismatched = 0;
        for (int32 RecordIndex = 0; RecordIndex < DressingRecordCount; ++RecordIndex)
        {
            const dressing::FDressingRecord& Record = dressing::kRecords[RecordIndex];
            if (TerrainAt(Record.X, Record.Y) != echoes::sim::Terrain::Blocked)
            {
                ++Mismatched;
            }
        }
        if (Mismatched > 0)
        {
            bDressingActive = false;
            DressingPlacedCount = 0;
            DressingRefusedCount = Mismatched;
            DressingInstancedCount = 0;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_DRESSING_INACTIVE] site=%s records=%d offBlocked=%d reason=liveTerrainIsNotTheBoundCompiledPack"),
                ANSI_TO_TCHAR(dressing::kSiteId),
                DressingRecordCount,
                Mismatched);
            return;
        }
    }
    DressingPlacedCount = 0;
    DressingRefusedCount = 0;
    DressingInstancedCount = 0;
    bool bShelvesChanged = false;
    bool bShardsChanged = false;
    const FTransform Hidden = HiddenTransform();
    for (int32 RecordIndex = 0; RecordIndex < DressingRecordCount; ++RecordIndex)
    {
        const dressing::FDressingRecord& Record = dressing::kRecords[RecordIndex];
        const int32 X = Record.X;
        const int32 Y = Record.Y;
        // Conformance is re-checked against the live simulation, never
        // trusted from the pack: the record exists only while its cell is
        // still Blocked.
        const bool bConformant =
            TerrainAt(X, Y) == echoes::sim::Terrain::Blocked;
        const bool bKnown =
            VisibilityAt(X, Y) != echoes::sim::Visibility::Unexplored;
        const bool bOpen = bConformant && ReshapedOpenAt(X, Y);
        if (bConformant)
        {
            ++DressingPlacedCount;
        }
        else
        {
            ++DressingRefusedCount;
            if (!DressingRefusalReported[RecordIndex])
            {
                DressingRefusalReported[RecordIndex] = true;
                UE_LOG(
                    LogEchoes,
                    Warning,
                    TEXT("[ECHOES_DRESSING_REFUSED] record=%s cell=%d reason=cellNotBlocked"),
                    ANSI_TO_TCHAR(Record.Id),
                    Record.CellIndex);
            }
        }
        const bool bDraw = bConformant && bKnown && !bOpen;
        if (bDraw)
        {
            ++DressingInstancedCount;
        }
        const uint8 State = bDraw ? 1 : 0;
        if (DressingDrawnState[RecordIndex] == State)
        {
            continue;
        }
        DressingDrawnState[RecordIndex] = State;
        const bool bShard = Record.Class == dressing::EDressingClass::GlassShard;
        UInstancedStaticMeshComponent* Layer =
            bShard ? DressingShards.Get() : DressingShelves.Get();
        Layer->UpdateInstanceTransform(
            DressingInstanceIndex[RecordIndex],
            bDraw ? DressingTransform(RecordIndex) : Hidden,
            false,
            false,
            true);
        (bShard ? bShardsChanged : bShelvesChanged) = true;
    }
    if (bShelvesChanged)
    {
        DressingShelves->MarkRenderStateDirty();
    }
    if (bShardsChanged)
    {
        DressingShards->MarkRenderStateDirty();
    }
}

bool AEchoesTerrainView::IsDressingRecordInstanced(int32 RecordIndex) const
{
    return bDressingActive &&
        DressingDrawnState.IsValidIndex(RecordIndex) &&
        DressingDrawnState[RecordIndex] == 1;
}

bool AEchoesTerrainView::AreDressingLayersPresentationOnly() const
{
    for (const UInstancedStaticMeshComponent* Layer :
         {DressingShelves.Get(), DressingShards.Get()})
    {
        if (Layer == nullptr ||
            Layer->GetCollisionEnabled() != ECollisionEnabled::NoCollision ||
            Layer->GetGenerateOverlapEvents() || Layer->CastShadow ||
            Layer->bReceivesDecals || Layer->CanEverAffectNavigation())
        {
            return false;
        }
    }
    return true;
}
