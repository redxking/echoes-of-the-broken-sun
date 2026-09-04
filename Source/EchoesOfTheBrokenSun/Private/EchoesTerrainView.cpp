#include "EchoesTerrainView.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesBattlefieldPresentation.h"
#include "EchoesGlassScarCompiledMapPack.h"
#include "EchoesGlassScarDressingPack.h"
#include "EchoesLumeReachDressingPack.h"
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

namespace gs_dressing = echoes::world::glass_scar_dressing;
namespace gs_map_pack = echoes::world::glass_scar_pack;
// The dressing records were verified against one exact compiled map pack;
// binding them to any other pack is refused at compile time.
static_assert(
    std::string_view(gs_dressing::kBaseCompiledPackSha256) ==
        std::string_view(gs_map_pack::kCompiledPackSha256),
    "Glass Scar dressing records were verified against a different compiled map pack");
static_assert(
    gs_dressing::kGridWidthTiles == gs_map_pack::kGridWidthTiles &&
        gs_dressing::kGridHeightTiles == gs_map_pack::kGridHeightTiles,
    "Glass Scar dressing grid disagrees with the compiled map pack grid");

namespace lr_dressing = echoes::world::lume_reach_dressing;
static_assert(
    lr_dressing::kGridWidthTiles == 64 && lr_dressing::kGridHeightTiles == 64,
    "Lume Reach dressing grid disagrees with the 64x64 grid");
static_assert(
    lr_dressing::kRecordCount == 39,
    "Lume Reach dressing record count must match authored contract");

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

    ChasmBanks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("ChasmBanks"));
    ChasmBanks->SetupAttachment(SceneRoot);
    ChasmTerrace = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("ChasmTerrace"));
    ChasmTerrace->SetupAttachment(SceneRoot);
    ChasmTeeth = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("ChasmTeeth"));
    ChasmTeeth->SetupAttachment(SceneRoot);
    ChasmBed = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("ChasmBed"));
    ChasmBed->SetupAttachment(SceneRoot);

    for (UInstancedStaticMeshComponent* Layer :
         {BlockedTiles, ScarredTiles, DressingShelves, DressingShards,
          ChasmBanks, ChasmTerrace, ChasmTeeth, ChasmBed})
    {
        Layer->SetMobility(EComponentMobility::Movable);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCastShadow(false);
        Layer->SetReceivesDecals(false);
        Layer->SetCanEverAffectNavigation(false);
    }
    // The cliff layers cast so the drop reads under the sun; tiles never did.
    ChasmBanks->SetCastShadow(true);
    ChasmTerrace->SetCastShadow(true);
    ChasmTeeth->SetCastShadow(true);

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
    // On a composed chasm the band rows are read as the drop itself; a ridge
    // tooth floating over the abyss would say the same thing worse.
    const bool bInChasmBand =
        bChasmComposed && TileY >= ChasmBandMinRow && TileY <= ChasmBandMaxRow;
    BlockedTiles->UpdateInstanceTransform(
        TileIndex,
        bKnown && Terrain == echoes::sim::Terrain::Blocked && !bInChasmBand
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
    std::optional<echoes::sim::PlayerId> ScopedPlayer,
    std::optional<EEchoesOperationMode> OperationMode)
{
    // A named player must exist, otherwise the caller believes it is scoping
    // the view when it is not.
    if (ScopedPlayer.has_value() &&
        Simulation.FindPlayer(*ScopedPlayer) == nullptr)
    {
        return false;
    }
    // If OperationMode is not explicitly supplied, check if the live terrain
    // matches the Lume Reach signature (four gated bastions along y=28):
    if (!OperationMode.has_value())
    {
        if (Simulation.Config().mapWidthTiles == 64 &&
            Simulation.Config().mapHeightTiles == 64 &&
            Simulation.TerrainAt(20, 28) == echoes::sim::Terrain::Blocked &&
            Simulation.TerrainAt(29, 28) == echoes::sim::Terrain::Blocked &&
            Simulation.TerrainAt(35, 28) == echoes::sim::Terrain::Blocked &&
            Simulation.TerrainAt(44, 28) == echoes::sim::Terrain::Blocked)
        {
            OperationMode = EEchoesOperationMode::CampaignChoirAtLumeReach;
        }
    }
    if (!InitializeScopedTerrain(
            Simulation.Config().mapWidthTiles,
            Simulation.Config().mapHeightTiles,
            TileWorldSize,
            MapPreset,
            OperationMode))
    {
        return false;
    }
    ScopedPlayerId = ScopedPlayer;
    if (MapPreset == EEchoesSkirmishMapPreset::GlassScar)
    {
        ComposeGlassScarChasm(
            [&Simulation](int32 X, int32 Y)
            {
                return Simulation.TerrainAt(X, Y);
            });
    }
    return SyncTerrain(Simulation);
}

bool AEchoesTerrainView::InitializeScopedTerrain(
    int32 InMapWidthTiles,
    int32 InMapHeightTiles,
    float TileWorldSize,
    EEchoesSkirmishMapPreset MapPreset,
    std::optional<EEchoesOperationMode> OperationMode)
{
    if (MapPreset != EEchoesSkirmishMapPreset::GlassScar &&
        MapPreset != EEchoesSkirmishMapPreset::CrownfallBasin &&
        MapPreset != EEchoesSkirmishMapPreset::SorynConfluence)
    {
        return false;
    }
    ActiveMapPreset = MapPreset;
    ActiveOperationMode = OperationMode;
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
    ClearChasmComposition();
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
    ActiveDressingRecords.Reset();
    ActiveDressingSiteId.Empty();
    ActiveDressingPackSha.Empty();
    ActiveDressingBasePackSha.Empty();
    DressingShelves->ClearInstances();
    DressingShards->ClearInstances();

    bool bIsLumeReach = false;
    if (ActiveOperationMode.has_value())
    {
        switch (*ActiveOperationMode)
        {
            case EEchoesOperationMode::CampaignPrologue:
            case EEchoesOperationMode::CampaignChoirAtLumeReach:
            case EEchoesOperationMode::CampaignNoNeutralLedger:
            case EEchoesOperationMode::CampaignFutureThatWon:
            case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
            case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
            case EEchoesOperationMode::CampaignTheBrokenSun:
                bIsLumeReach = true;
                break;
            default:
                bIsLumeReach = false;
                break;
        }
    }

    if (bIsLumeReach)
    {
        ActiveDressingProfile = EDressingSiteProfile::LumeReach;
    }
    else if (ActiveMapPreset == EEchoesSkirmishMapPreset::GlassScar)
    {
        ActiveDressingProfile = EDressingSiteProfile::GlassScar;
    }
    else
    {
        ActiveDressingProfile = EDressingSiteProfile::None;
        return true;
    }

    if (ActiveDressingProfile == EDressingSiteProfile::GlassScar)
    {
        if (MapWidthTiles != gs_dressing::kGridWidthTiles ||
            MapHeightTiles != gs_dressing::kGridHeightTiles)
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
                ANSI_TO_TCHAR(gs_dressing::kSiteId),
                ScarredMesh != nullptr ? TEXT("ready") : TEXT("missing"),
                ShardMesh != nullptr ? TEXT("ready") : TEXT("missing"),
                BlockedMaterials.Num());
            return false;
        }

        ActiveDressingSiteId = ANSI_TO_TCHAR(gs_dressing::kSiteId);
        ActiveDressingPackSha = ANSI_TO_TCHAR(gs_dressing::kPackSha256);
        ActiveDressingBasePackSha = ANSI_TO_TCHAR(gs_dressing::kBaseCompiledPackSha256);
        DressingRecordCount = gs_dressing::kRecordCount;

        DressingShelves->SetStaticMesh(ScarredMesh);
        DressingShards->SetStaticMesh(ShardMesh);
        for (int32 MaterialIndex = 0; MaterialIndex < BlockedMaterials.Num();
             ++MaterialIndex)
        {
            DressingShelves->SetMaterial(MaterialIndex, BlockedMaterials[MaterialIndex]);
            DressingShards->SetMaterial(MaterialIndex, BlockedMaterials[MaterialIndex]);
        }

        ActiveDressingRecords.Reserve(DressingRecordCount);
        for (int32 RecordIndex = 0; RecordIndex < DressingRecordCount; ++RecordIndex)
        {
            const gs_dressing::FDressingRecord& Record = gs_dressing::kRecords[RecordIndex];
            FActiveDressingRecord ActiveRec;
            ActiveRec.bIsShardLayer = (Record.Class == gs_dressing::EDressingClass::GlassShard);
            ActiveRec.X = Record.X;
            ActiveRec.Y = Record.Y;
            ActiveRec.OrientationOrdinal = Record.OrientationOrdinal;
            ActiveRec.ScaleBand = Record.ScaleBand;
            ActiveRec.CellIndex = Record.CellIndex;
            ActiveRec.Id = Record.Id;
            ActiveDressingRecords.Add(ActiveRec);
        }
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::LumeReach)
    {
        if (MapWidthTiles != lr_dressing::kGridWidthTiles ||
            MapHeightTiles != lr_dressing::kGridHeightTiles)
        {
            return true;
        }
        if (ShardMesh == nullptr || ScarredMesh == nullptr ||
            AuthoredSurfaceMaterial == nullptr)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_DRESSING_REFUSED] site=%s reason=assets shelf=%s shard=%s material=%s"),
                ANSI_TO_TCHAR(lr_dressing::kSiteId),
                ScarredMesh != nullptr ? TEXT("ready") : TEXT("missing"),
                ShardMesh != nullptr ? TEXT("ready") : TEXT("missing"),
                AuthoredSurfaceMaterial != nullptr ? TEXT("ready") : TEXT("missing"));
            return false;
        }

        ActiveDressingSiteId = ANSI_TO_TCHAR(lr_dressing::kSiteId);
        ActiveDressingPackSha = ANSI_TO_TCHAR(lr_dressing::kPackSha256);
        ActiveDressingBasePackSha = ANSI_TO_TCHAR(lr_dressing::kBaseCompiledPackSha256);
        DressingRecordCount = lr_dressing::kRecordCount;

        DressingShelves->SetStaticMesh(ScarredMesh);
        DressingShards->SetStaticMesh(ShardMesh);

        // Lume Reach Pale Ceramic civic plates and Broken-Sun Amber warm conduit light:
        // Slot 0: Charcoal foundation
        // Slot 1: Pale Ceramic civic plates
        // Slot 2: Recess dark
        // Slot 3: Broken-Sun Amber warm interior window light
        // All roughness >= 0.85 floor strictly preserved.
        const FLinearColor LumeColors[4] = {
            FLinearColor(0.025f, 0.028f, 0.032f),
            FLinearColor(0.68f, 0.66f, 0.62f),
            FLinearColor(0.05f, 0.05f, 0.06f),
            FLinearColor(0.92f, 0.52f, 0.06f)
        };
        const float RoughnessValues[4] = {0.88f, 0.85f, 0.90f, 0.85f};
        const float MetallicValues[4] = {0.05f, 0.08f, 0.02f, 0.05f};
        const float EmissiveValues[4] = {0.0f, 0.0f, 0.0f, 1.2f};

        for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
        {
            UMaterialInstanceDynamic* DynamicMat =
                UMaterialInstanceDynamic::Create(AuthoredSurfaceMaterial, this);
            if (DynamicMat != nullptr)
            {
                DynamicMat->SetVectorParameterValue(TerrainColorParameterName, LumeColors[MaterialIndex]);
                DynamicMat->SetScalarParameterValue(TerrainMetallicParameterName, MetallicValues[MaterialIndex]);
                DynamicMat->SetScalarParameterValue(TerrainRoughnessParameterName, RoughnessValues[MaterialIndex]);
                DynamicMat->SetScalarParameterValue(TerrainEmissiveParameterName, EmissiveValues[MaterialIndex]);
                DressingShelves->SetMaterial(MaterialIndex, DynamicMat);
                DressingShards->SetMaterial(MaterialIndex, DynamicMat);
            }
        }

        ActiveDressingRecords.Reserve(DressingRecordCount);
        for (int32 RecordIndex = 0; RecordIndex < DressingRecordCount; ++RecordIndex)
        {
            const lr_dressing::FDressingRecord& Record = lr_dressing::kRecords[RecordIndex];
            FActiveDressingRecord ActiveRec;
            ActiveRec.bIsShardLayer = (Record.Class == lr_dressing::EDressingClass::ConduitPylon);
            ActiveRec.X = Record.X;
            ActiveRec.Y = Record.Y;
            ActiveRec.OrientationOrdinal = Record.OrientationOrdinal;
            ActiveRec.ScaleBand = Record.ScaleBand;
            ActiveRec.CellIndex = Record.CellIndex;
            ActiveRec.Id = Record.Id;
            ActiveDressingRecords.Add(ActiveRec);
        }
    }

    const FTransform Hidden = HiddenTransform();
    DressingInstanceIndex.Init(INDEX_NONE, DressingRecordCount);
    DressingDrawnState.Init(DressingNeverDrawn, DressingRecordCount);
    DressingRefusalReported.Init(false, DressingRecordCount);
    for (int32 RecordIndex = 0; RecordIndex < DressingRecordCount; ++RecordIndex)
    {
        const FActiveDressingRecord& Record = ActiveDressingRecords[RecordIndex];
        UInstancedStaticMeshComponent* Layer =
            Record.bIsShardLayer
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
        *ActiveDressingSiteId,
        DressingRecordCount,
        *ActiveDressingPackSha,
        *ActiveDressingBasePackSha);
    return true;
}

FTransform AEchoesTerrainView::DressingTransform(int32 RecordIndex) const
{
    const FActiveDressingRecord& Record = ActiveDressingRecords[RecordIndex];
    const float HalfWidth = static_cast<float>(MapWidthTiles) * 0.5f;
    const float HalfHeight = static_cast<float>(MapHeightTiles) * 0.5f;
    const int32 Band = FMath::Clamp<int32>(Record.ScaleBand, 0, 2);
    const FRotator Rotation(
        0.0f,
        90.0f * static_cast<float>(Record.OrientationOrdinal % 4),
        0.0f);
    // Records on the chasm band stand on the bed: they still mark the cell
    // Blocked, now as floor debris at the bottom of the drop.
    const bool bOnChasmBed =
        bChasmComposed && Record.Y >= ChasmBandMinRow && Record.Y <= ChasmBandMaxRow;
    const FVector Location(
        (static_cast<float>(Record.X) - HalfWidth) * WorldUnitsPerTile,
        (static_cast<float>(Record.Y) - HalfHeight) * WorldUnitsPerTile,
        (Record.bIsShardLayer ? 10.0f : 4.0f) + (bOnChasmBed ? ChasmBedTopZ : 0.0f));
    if (Record.bIsShardLayer)
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
            const FActiveDressingRecord& Record = ActiveDressingRecords[RecordIndex];
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
                *ActiveDressingSiteId,
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
        const FActiveDressingRecord& Record = ActiveDressingRecords[RecordIndex];
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
        const bool bShard = Record.bIsShardLayer;
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

// ---------------------------------------------------------------------------
// Glass Scar chasm composition (directive gate 50)
// ---------------------------------------------------------------------------

void AEchoesTerrainView::ConfigureChasmLayer(
    UInstancedStaticMeshComponent* Layer,
    UStaticMesh* Mesh,
    const FLinearColor& BaseColor,
    const FLinearColor& GlowColor,
    TArray<TObjectPtr<UMaterialInstanceDynamic>>& Materials)
{
    Layer->ClearInstances();
    Layer->SetStaticMesh(Mesh);
    Materials.Reset();
    // Same four-slot reading as the environment accents: plate, foundation,
    // pale relief, emissive fissure. Slot 3 is the amber fissure, kept off
    // every owner, faction, resource, and command identity hue.
    const FLinearColor Palette[] = {
        BaseColor,
        FLinearColor(BaseColor.R * 0.22f, BaseColor.G * 0.22f, BaseColor.B * 0.25f),
        FLinearColor(
            FMath::Min(BaseColor.R * 1.75f + 0.04f, 1.0f),
            FMath::Min(BaseColor.G * 1.75f + 0.04f, 1.0f),
            FMath::Min(BaseColor.B * 1.75f + 0.04f, 1.0f)),
        GlowColor};
    for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
    {
        UMaterialInstanceDynamic* Material =
            UMaterialInstanceDynamic::Create(AuthoredSurfaceMaterial, this);
        if (Material == nullptr)
        {
            continue;
        }
        Material->SetVectorParameterValue(TerrainColorParameterName, Palette[MaterialIndex]);
        // The walking plate is matte like the collision floor it replaces, so
        // the gameplay camera's steep view does not catch a sun sheen across
        // the whole basin; the foundation slot keeps the vitrified gloss for
        // the cliff faces.
        Material->SetScalarParameterValue(
            TerrainMetallicParameterName, MaterialIndex == 1 ? 0.46f : 0.03f);
        Material->SetScalarParameterValue(
            TerrainRoughnessParameterName, MaterialIndex == 1 ? 0.18f : 0.94f);
        Material->SetScalarParameterValue(
            TerrainEmissiveParameterName, MaterialIndex == 3 ? 1.8f : 0.0f);
        Materials.Add(Material);
        Layer->SetMaterial(MaterialIndex, Material);
    }
}

void AEchoesTerrainView::ClearChasmComposition()
{
    bChasmComposed = false;
    ChasmBandMinRow = -1;
    ChasmBandMaxRow = -1;
    for (UInstancedStaticMeshComponent* Layer :
         {ChasmBanks, ChasmTerrace, ChasmTeeth, ChasmBed})
    {
        if (Layer != nullptr)
        {
            Layer->ClearInstances();
        }
    }
    for (UPointLightComponent* Light : ChasmFissureLights)
    {
        if (Light != nullptr)
        {
            Light->DestroyComponent();
        }
    }
    ChasmFissureLights.Reset();
}

int32 AEchoesTerrainView::GetChasmInstanceCount() const
{
    int32 Count = 0;
    for (const UInstancedStaticMeshComponent* Layer :
         {ChasmBanks, ChasmTerrace, ChasmTeeth, ChasmBed})
    {
        Count += Layer != nullptr ? Layer->GetInstanceCount() : 0;
    }
    return Count;
}

void AEchoesTerrainView::SetTileLayersVisible(bool bVisible)
{
    for (UInstancedStaticMeshComponent* Layer :
         {BlockedTiles, ScarredTiles, DressingShelves, DressingShards})
    {
        if (Layer != nullptr)
        {
            Layer->SetVisibility(bVisible, true);
        }
    }
}

bool AEchoesTerrainView::ComposeGlassScarChasm(
    TFunctionRef<echoes::sim::Terrain(int32, int32)> TerrainAt)
{
    ClearChasmComposition();
    if (ScarredMesh == nullptr || BlockedMesh == nullptr ||
        AuthoredSurfaceMaterial == nullptr || MapWidthTiles < 16 ||
        MapHeightTiles < 16)
    {
        return false;
    }

    // 1. Find the scar band: contiguous rows that are mostly Blocked while both
    //    map edges stay passable (the edge corridors). Anything else - a
    //    campaign layout, another preset's terrain - composes nothing.
    const int32 MinBlockedPerRow = (MapWidthTiles * 35) / 100;
    int32 MinRow = -1;
    int32 MaxRow = -1;
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        int32 Blocked = 0;
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            Blocked += TerrainAt(TileX, TileY) == echoes::sim::Terrain::Blocked ? 1 : 0;
        }
        const bool bBandRow =
            Blocked >= MinBlockedPerRow &&
            TerrainAt(0, TileY) != echoes::sim::Terrain::Blocked &&
            TerrainAt(MapWidthTiles - 1, TileY) != echoes::sim::Terrain::Blocked;
        if (!bBandRow)
        {
            if (MinRow >= 0)
            {
                break;
            }
            continue;
        }
        if (MinRow < 0)
        {
            MinRow = TileY;
        }
        MaxRow = TileY;
    }
    const int32 BandRows = MinRow >= 0 ? MaxRow - MinRow + 1 : 0;
    if (BandRows < 3 || BandRows > 9)
    {
        return false;
    }

    // 2. Classify columns across the band: deep where every band row is
    //    Blocked, open where a crossing or corridor passes.
    TArray<bool> DeepColumn;
    DeepColumn.Init(false, MapWidthTiles);
    for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
    {
        bool bDeep = true;
        for (int32 TileY = MinRow; TileY <= MaxRow && bDeep; ++TileY)
        {
            bDeep = TerrainAt(TileX, TileY) == echoes::sim::Terrain::Blocked;
        }
        DeepColumn[TileX] = bDeep;
    }
    struct FSpan
    {
        float MinX;
        float MaxX;
        bool bDeep;
        bool bTouchesEdge;
    };
    TArray<FSpan> Spans;
    const float HalfWidth = static_cast<float>(MapWidthTiles) * 0.5f;
    const float HalfHeight = static_cast<float>(MapHeightTiles) * 0.5f;
    const float T = WorldUnitsPerTile;
    const auto ColumnMin = [&](int32 TileX) { return (static_cast<float>(TileX) - HalfWidth) * T - T * 0.5f; };
    const auto ColumnMax = [&](int32 TileX) { return (static_cast<float>(TileX) - HalfWidth) * T + T * 0.5f; };
    for (int32 TileX = 0; TileX < MapWidthTiles;)
    {
        int32 End = TileX;
        while (End + 1 < MapWidthTiles && DeepColumn[End + 1] == DeepColumn[TileX])
        {
            ++End;
        }
        Spans.Add({ColumnMin(TileX), ColumnMax(End), DeepColumn[TileX],
                   TileX == 0 || End == MapWidthTiles - 1});
        TileX = End + 1;
    }
    int32 DeepSpans = 0;
    for (const FSpan& Span : Spans)
    {
        DeepSpans += Span.bDeep ? 1 : 0;
    }
    if (DeepSpans == 0)
    {
        return false;
    }

    const float BandMinY = (static_cast<float>(MinRow) - HalfHeight) * T - T * 0.5f;
    const float BandMaxY = (static_cast<float>(MaxRow) - HalfHeight) * T + T * 0.5f;
    const float BandCenterY = 0.5f * (BandMinY + BandMaxY);
    const float BandHalfWidth = 0.5f * (BandMaxY - BandMinY);
    const float MapMinX = -HalfWidth * T;
    const float MapMaxX = HalfWidth * T;
    const float MapMinY = -HalfHeight * T;
    const float MapMaxY = HalfHeight * T;

    // The plate takes the retired collision floor's albedo exactly: under the
    // A1 exposure rig that albedo is what gate 3 accepted as charcoal
    // vitrified ground, and anything brighter reads as pale at gameplay pitch.
    const FLinearColor BankColor(0.018f, 0.027f, 0.032f);
    const FLinearColor TerraceColor(0.016f, 0.022f, 0.027f);
    const FLinearColor BedColor(0.030f, 0.026f, 0.024f);
    const FLinearColor FissureGlow(1.0f, 0.42f, 0.08f);
    ConfigureChasmLayer(ChasmBanks, ScarredMesh, BankColor, FissureGlow, ChasmBankMaterials);
    ConfigureChasmLayer(ChasmTerrace, ScarredMesh, TerraceColor, FissureGlow, ChasmBankMaterials);
    ConfigureChasmLayer(ChasmTeeth, BlockedMesh, FLinearColor(0.030f, 0.040f, 0.048f), FissureGlow, ChasmTeethMaterials);
    ConfigureChasmLayer(ChasmBed, ScarredMesh, BedColor, FissureGlow, ChasmBedMaterials);

    // 3. Banks: overlapping shelf plates from each rim out to the map edge.
    //    The shelf is 780 wide; plates overlap by ~60 and alternate 6 lower
    //    so seams do not show. Row 0 is deep enough to reach the bed.
    constexpr float ShelfSource = 780.0f;
    constexpr float PlateScaleXY = 2.05f;
    constexpr float PlateWidth = ShelfSource * PlateScaleXY;
    constexpr float PlateSpacing = PlateWidth - 60.0f;
    constexpr float PlateTopLocal = 39.0f;
    constexpr float PlateTopZ = -6.0f; // just under the deck of a crossing
    constexpr float RimRowScaleZ = 1.9f;
    const int32 TilesAcross = FMath::CeilToInt((MapMaxX - MapMinX) / PlateSpacing) + 1;
    const float FirstTileX = -0.5f * static_cast<float>(TilesAcross - 1) * PlateSpacing;
    for (const float BankSign : {-1.0f, 1.0f})
    {
        const float RimY = BankSign < 0.0f ? BandMinY : BandMaxY;
        const float BankExtent = BankSign < 0.0f ? RimY - MapMinY : MapMaxY - RimY;
        const int32 Rows = FMath::CeilToInt(BankExtent / PlateSpacing) + 1;
        for (int32 Row = 0; Row < Rows; ++Row)
        {
            const float ScaleZ = Row == 0 ? RimRowScaleZ : 1.0f;
            const float CenterY =
                RimY + BankSign * (PlateWidth * 0.5f + static_cast<float>(Row) * PlateSpacing);
            for (int32 Column = 0; Column < TilesAcross; ++Column)
            {
                const float CenterX = FirstTileX + static_cast<float>(Column) * PlateSpacing;
                const float Stagger = ((Row + Column) % 2 == 0) ? 0.0f : -6.0f;
                ChasmBanks->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    FVector(CenterX, CenterY, PlateTopZ - PlateTopLocal * ScaleZ + Stagger),
                    FVector(PlateScaleXY, PlateScaleXY, ScaleZ)));
            }
        }
    }

    // 4. Terrace: a narrower step halfway down each cliff so the inner face
    //    reads as strata rather than one flat wall.
    constexpr float TerraceInset = 240.0f;
    constexpr float TerraceScaleZ = 1.2f;
    constexpr float TerraceTopZ = -330.0f;
    for (const float BankSign : {-1.0f, 1.0f})
    {
        const float CenterY =
            BandCenterY + BankSign * (BandHalfWidth - TerraceInset + PlateWidth * 0.5f);
        for (int32 Column = 0; Column < TilesAcross; ++Column)
        {
            const float CenterX =
                FirstTileX + static_cast<float>(Column) * PlateSpacing + 260.0f * BankSign;
            ChasmTerrace->AddInstance(FTransform(
                FRotator(0.0f, 3.0f * BankSign, 0.0f),
                FVector(CenterX, CenterY, TerraceTopZ - PlateTopLocal * TerraceScaleZ),
                FVector(PlateScaleXY, PlateScaleXY, TerraceScaleZ)));
        }
    }

    // 5. Bed under the deep spans; a ground-level plate under an edge
    //    corridor, which the pack keeps passable and units cross at z = 0.
    constexpr float BedScaleZ = 0.5f;
    const float BedWidthY = 2.0f * (BandHalfWidth - TerraceInset) + 260.0f;
    for (const FSpan& Span : Spans)
    {
        const float SpanWidth = Span.MaxX - Span.MinX;
        const float CenterX = 0.5f * (Span.MinX + Span.MaxX);
        if (Span.bDeep)
        {
            ChasmBed->AddInstance(FTransform(
                FRotator::ZeroRotator,
                FVector(CenterX, BandCenterY, ChasmBedTopZ - PlateTopLocal * BedScaleZ),
                FVector((SpanWidth + 120.0f) / ShelfSource, BedWidthY / ShelfSource, BedScaleZ)));
        }
        else if (Span.bTouchesEdge)
        {
            ChasmBed->AddInstance(FTransform(
                FRotator::ZeroRotator,
                FVector(CenterX, BandCenterY, PlateTopZ - PlateTopLocal),
                FVector((SpanWidth + 40.0f) / ShelfSource, (2.0f * BandHalfWidth) / ShelfSource, 1.0f)));
        }
    }

    // 6. Rim teeth along both cliff edges, skipping the open spans so each
    //    crossing reads clean.
    constexpr float ToothSpacing = 420.0f;
    int32 Tooth = 0;
    for (const float BankSign : {-1.0f, 1.0f})
    {
        const float RimY = (BankSign < 0.0f ? BandMinY : BandMaxY) - BankSign * 70.0f;
        for (float ToothX = MapMinX + 210.0f; ToothX < MapMaxX; ToothX += ToothSpacing, ++Tooth)
        {
            const float JitteredX = ToothX + ((Tooth % 2 == 0) ? 60.0f : -80.0f) * BankSign;
            bool bOpen = false;
            for (const FSpan& Span : Spans)
            {
                if (!Span.bDeep && JitteredX > Span.MinX - 120.0f && JitteredX < Span.MaxX + 120.0f)
                {
                    bOpen = true;
                    break;
                }
            }
            if (bOpen)
            {
                continue;
            }
            const float Scale = 0.55f + 0.2f * static_cast<float>(Tooth % 3);
            ChasmTeeth->AddInstance(FTransform(
                FRotator(0.0f, 37.0f * static_cast<float>(Tooth) + (BankSign < 0.0f ? 0.0f : 180.0f), 0.0f),
                FVector(JitteredX, RimY, -8.0f),
                FVector(Scale, Scale, Scale)));
        }
    }

    // 7. Fissure light in the deep spans so the bed carries amber.
    for (const FSpan& Span : Spans)
    {
        if (!Span.bDeep)
        {
            continue;
        }
        const float SpanWidth = Span.MaxX - Span.MinX;
        const int32 Lights = FMath::Max(1, FMath::RoundToInt(SpanWidth / 1300.0f));
        for (int32 Index = 0; Index < Lights; ++Index)
        {
            const float X = Span.MinX + SpanWidth * (static_cast<float>(Index) + 0.5f) / static_cast<float>(Lights);
            UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
            if (Light == nullptr)
            {
                continue;
            }
            Light->SetupAttachment(SceneRoot);
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetRelativeLocation(FVector(X, BandCenterY, ChasmBedTopZ + 130.0f));
            Light->SetLightColor(FLinearColor(1.0f, 0.38f, 0.05f));
            Light->SetIntensity(3800.0f);
            Light->SetAttenuationRadius(1500.0f);
            Light->SetSourceRadius(160.0f);
            Light->SetCastShadows(false);
            Light->RegisterComponent();
            ChasmFissureLights.Add(Light);
        }
    }

    for (UInstancedStaticMeshComponent* Layer :
         {ChasmBanks, ChasmTerrace, ChasmTeeth, ChasmBed})
    {
        Layer->MarkRenderStateDirty();
    }
    ChasmBandMinRow = MinRow;
    ChasmBandMaxRow = MaxRow;
    bChasmComposed = true;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_GLASS_SCAR_CHASM_READY] bandRows=%d-%d deepSpans=%d openSpans=%d banks=%d terrace=%d teeth=%d bed=%d fissureLights=%d bedTopZ=%.0f collisionAuthority=false routeAuthority=false"),
        MinRow,
        MaxRow,
        DeepSpans,
        Spans.Num() - DeepSpans,
        ChasmBanks->GetInstanceCount(),
        ChasmTerrace->GetInstanceCount(),
        ChasmTeeth->GetInstanceCount(),
        ChasmBed->GetInstanceCount(),
        ChasmFissureLights.Num(),
        ChasmBedTopZ);
    return true;
}
