#include "EchoesTerrainView.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesCliffMesh.h"
#include "ProceduralMeshComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesBattlefieldPresentation.h"
#include "EchoesCampaignTerrainBinding.h"
#include "../../../Content/World/Generated/Presentation/EchoesMissionLandmarks.h"
#include "EchoesGlassScarCompiledMapPack.h"
#include "EchoesGlassScarDressingPack.h"
#include "EchoesLumeReachDressingPack.h"
#include "EchoesOfTheBrokenSun.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
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

namespace mission_landmarks = echoes::world::mission_landmarks;
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

const mission_landmarks::Pack* ActiveMissionLandmarkPack(
    const std::optional<EEchoesOperationMode>& Operation)
{
    if (!Operation.has_value()) return nullptr;
    EEchoesCampaignMissionId Mission;
    if (!UEchoesSimulationSubsystem::GetMissionIdForOperation(*Operation, Mission)) return nullptr;
    const uint8 Ordinal = static_cast<uint8>(Mission);
    for (const auto& Pack : mission_landmarks::kPacks)
        if (Pack.mission_ordinal == Ordinal) return &Pack;
    return nullptr;
}
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
    BiomeGround = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BiomeGround"));
    BiomeGround->SetupAttachment(SceneRoot);
    BiomeSurface = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BiomeSurface"));
    BiomeSurface->SetupAttachment(SceneRoot);
    BiomeHorizon = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BiomeHorizon"));
    BiomeHorizon->SetupAttachment(SceneRoot);
    M01ExteriorSkirt = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("M01ExteriorSkirt"));
    M01ExteriorSkirt->SetupAttachment(SceneRoot);
    ContinuousCliffs = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("M01ContinuousCliffs"));
    ContinuousCliffs->SetupAttachment(SceneRoot);
    ContinuousCliffs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ContinuousCliffs->SetCollisionResponseToAllChannels(ECR_Ignore);
    ContinuousCliffs->SetCanEverAffectNavigation(false);
    ContinuousCliffs->SetGenerateOverlapEvents(false);
    ContinuousCliffs->SetCastShadow(false);
    ContinuousCliffs->SetReceivesDecals(false);
    M01ExteriorBanks = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("M01ExteriorBanks"));
    M01ExteriorBanks->SetupAttachment(SceneRoot);
    M01ExteriorBanks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    M01ExteriorBanks->SetCollisionResponseToAllChannels(ECR_Ignore);
    M01ExteriorBanks->SetCanEverAffectNavigation(false);
    M01ExteriorBanks->SetGenerateOverlapEvents(false);
    M01ExteriorBanks->SetCastShadow(false);
    M01ExteriorBanks->SetReceivesDecals(false);

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
         {BlockedTiles, ScarredTiles, BiomeGround, BiomeSurface, BiomeHorizon, M01ExteriorSkirt, DressingShelves, DressingShards,
          ChasmBanks, ChasmTerrace, ChasmTeeth, ChasmBed})
    {
        Layer->SetMobility(EComponentMobility::Movable);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCastShadow(false);
        Layer->SetReceivesDecals(false);
        Layer->SetCanEverAffectNavigation(false);
    }
    for (const TCHAR* Name : {TEXT("M01ArchiveCradle"), TEXT("M01ArchiveFrame"),
                              TEXT("M01RoutePaving"), TEXT("M01ServiceConduit"), TEXT("M01ArchiveApron"), TEXT("M01ArchiveLoadingFace")})
    {
        auto* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
        Layer->SetupAttachment(SceneRoot);
        Layer->SetMobility(EComponentMobility::Movable);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCastShadow(false);
        Layer->SetReceivesDecals(false);
        Layer->SetCanEverAffectNavigation(false);
        MissionLandmarkLayers.Add(Layer);
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
    const bool bNaturalFormation = ActiveDressingProfile == EDressingSiteProfile::GlassScar ||
        ActiveDressingProfile == EDressingSiteProfile::SubterraneanCaverns ||
        ActiveDressingProfile == EDressingSiteProfile::ShivergrassBasin;
    const float HeightVariation = bNaturalFormation
        ? .85f + .18f * FMath::Sin(TileX * .47f + TileY * .31f) + .08f * FMath::Cos(TileY * 1.73f)
        : 1.0f;
    return FTransform(
        FRotator::ZeroRotator,
        FVector(
            (static_cast<float>(TileX) - HalfWidth) * WorldUnitsPerTile,
            (static_cast<float>(TileY) - HalfHeight) * WorldUnitsPerTile,
            bBlocked ? 8.0f : 0.0f),
        bBlocked
            ? FVector(PlanarScale, PlanarScale, 0.82f * HeightVariation)
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
    // Glass Scar's continuous-cliff compositor replaces the ordinary blocked
    // tile layer. Other mission packs keep their generic blocked terrain and
    // replace only the cells occupied by their explicit landmark footprint.
    const mission_landmarks::Pack* LandmarkPack =
        ActiveMissionLandmarkPack(ActiveOperationMode);
    const bool bM01ContinuousCliffs = bMissionLandmarksActive &&
        LandmarkPack != nullptr && LandmarkPack->mission_ordinal == 1;
    BlockedTiles->UpdateInstanceTransform(
        TileIndex,
        bKnown && Terrain == echoes::sim::Terrain::Blocked && !bInChasmBand &&
            !bM01ContinuousCliffs && !MissionLandmarkSolidCells.Contains(TileIndex)
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
    const float Scale = WorldUnitsPerTile / 200.0f;
    const bool bGroundKnown = bKnown && Terrain != echoes::sim::Terrain::Blocked && !bInChasmBand;
    uint32 PatchSeed = static_cast<uint32>(TileX) * 73856093u ^
        static_cast<uint32>(TileY) * 19349663u;
    PatchSeed ^= PatchSeed >> 16;
    PatchSeed *= 0x7feb352du;
    PatchSeed ^= PatchSeed >> 15;
    const bool bNaturalGround = ActiveDressingProfile == EDressingSiteProfile::GlassScar ||
        ActiveDressingProfile == EDressingSiteProfile::SubterraneanCaverns;
    const bool bGroundPatch = !bNaturalGround || PatchSeed % 100 < 34;
    const float PatchScale = bNaturalGround ? .65f + (PatchSeed % 37) * .01f : 1.0f;
    const float WorldX = (TileX - MapWidthTiles * .5f) * WorldUnitsPerTile;
    const float WorldY = (TileY - MapHeightTiles * .5f) * WorldUnitsPerTile;
    const bool bAuthoredRoute = bChasmComposed && FMath::Abs(WorldY) < 1100.0f &&
        (FMath::Abs(WorldX) < 720.0f || FMath::Abs(WorldX + 3800.0f) < 450.0f ||
         FMath::Abs(WorldX - 3400.0f) < 460.0f);
    // This substrate replaces the visible Engine cube, without touching pointer
    // traces. Chasm crossings retain their authored decks, not a false floor.
    BiomeSurface->UpdateInstanceTransform(TileIndex,
        bKnown && (!bInChasmBand || Terrain != echoes::sim::Terrain::Blocked)
            ? FTransform(FRotator::ZeroRotator, FVector(WorldX, WorldY, 0), FVector(Scale))
            : Hidden, false, false, true);
    BiomeGround->UpdateInstanceTransform(TileIndex,
        bGroundKnown && !bAuthoredRoute && bGroundPatch && !MissionLandmarkPavingCells.Contains(TileIndex)
            ? FTransform(FRotator(0, bNaturalGround ? PatchSeed % 360 : (PatchSeed % 4) * 90.0f, 0),
                FVector((TileX - MapWidthTiles * .5f) * WorldUnitsPerTile,
                        (TileY - MapHeightTiles * .5f) * WorldUnitsPerTile, 1.0f),
                FVector(Scale * PatchScale, Scale * PatchScale, Scale))
            : Hidden, false, false, true);

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
    if (!OperationMode.has_value() && !ScopedPlayer.has_value())
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
    if (MapPreset == EEchoesSkirmishMapPreset::GlassScar &&
        ActiveDressingProfile == EDressingSiteProfile::GlassScar)
    {
        // Static substrate comes from the authored scenario, never from hidden
        // changes at a replay seek tick. Cached player knowledge gates drawing.
        ComposeGlassScarChasm(
            [&Simulation, MapPreset, OperationMode, ScopedPlayer](int32 X, int32 Y)
            {
                if (OperationMode == EEchoesOperationMode::CampaignPrologue)
                    return echoes::world::IsCampaignTerrainPassable(1,
                        echoes::sim::FutureWellChoice::Preserve, X, Y)
                        ? echoes::sim::Terrain::Open : echoes::sim::Terrain::Blocked;
                if (OperationMode == EEchoesOperationMode::Skirmish || ScopedPlayer.has_value())
                    return FEchoesSkirmishSetupModel::IsBlockedTile(MapPreset, X, Y)
                        ? echoes::sim::Terrain::Blocked : echoes::sim::Terrain::Open;
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
        const float Emissive = MaterialIndex == 3 ? 0.12f : 0.0f;
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
                0.03f);
            Material->SetScalarParameterValue(
                TerrainRoughnessParameterName,
                0.90f);
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
    BiomeGround->ClearInstances();
    BiomeSurface->ClearInstances();
    BlockedTiles->PreAllocateInstancesMemory(TileCount);
    ScarredTiles->PreAllocateInstancesMemory(TileCount);
    BiomeGround->PreAllocateInstancesMemory(TileCount);
    BiomeSurface->PreAllocateInstancesMemory(TileCount);
    const FTransform Hidden = HiddenTransform();
    for (int32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
    {
        BlockedTiles->AddInstance(Hidden, false);
        ScarredTiles->AddInstance(Hidden, false);
        BiomeGround->AddInstance(Hidden, false);
        BiomeSurface->AddInstance(Hidden, false);
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
        BiomeGround->MarkRenderStateDirty();
        BiomeSurface->MarkRenderStateDirty();
        SyncChasmVisibility();
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

    // The shared adapter serves both live play and detached replay. Explored
    // terrain and passability must remain the selected player's last knowledge.
    const std::optional<echoes::sim::PlayerView> PlayerView = ScopedPlayerId.has_value()
        ? Simulation.CreatePlayerView(*ScopedPlayerId) : std::nullopt;
    if (ScopedPlayerId.has_value() && !PlayerView.has_value()) return false;
    const auto PresentedTerrainAt = [&Simulation, &PlayerView](int32 X, int32 Y)
    {
        return PlayerView ? PlayerView->TerrainAt(X, Y) : Simulation.TerrainAt(X, Y);
    };
    const auto PresentedVisibilityAt = [&PlayerView](int32 X, int32 Y)
    {
        return PlayerView ? PlayerView->VisibilityAt(echoes::sim::Vec2::FromTiles(X, Y))
            : echoes::sim::Visibility::Visible;
    };
    const auto PresentedPassableAt = [&Simulation, &PlayerView](int32 X, int32 Y)
    {
        const auto Position = echoes::sim::Vec2::FromTiles(X, Y);
        return PlayerView ? PlayerView->IsPositionPassable(Position) : Simulation.IsPositionPassable(Position);
    };

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
            const echoes::sim::Terrain Terrain = PresentedTerrainAt(TileX, TileY);
            const echoes::sim::Visibility Visibility = PresentedVisibilityAt(TileX, TileY);
            const bool bKnown =
                Visibility != echoes::sim::Visibility::Unexplored;
            // Diagnostic census retains its documented authoritative meaning;
            // only the scoped values below are used by rendering or dressing.
            const auto AuthoritativeTerrain = Simulation.TerrainAt(TileX, TileY);
            BlockedTileCount += AuthoritativeTerrain == echoes::sim::Terrain::Blocked ? 1 : 0;
            ScarredTileCount += AuthoritativeTerrain == echoes::sim::Terrain::Scarred ? 1 : 0;
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
        BiomeGround->MarkRenderStateDirty();
        BiomeSurface->MarkRenderStateDirty();
        SyncChasmVisibility();
    }
    SyncDressingWith(PresentedTerrainAt, PresentedVisibilityAt, PresentedPassableAt);
    return true;
}

bool AEchoesTerrainView::ConfigureWorldKit()
{
    const TCHAR* Family = TEXT("Basalt");
    switch (ActiveDressingProfile)
    {
        case EDressingSiteProfile::ShivergrassBasin: Family = TEXT("Shivergrass"); break;
        case EDressingSiteProfile::SubterraneanCaverns: Family = TEXT("Cavern"); break;
        case EDressingSiteProfile::LumeReach:
        case EDressingSiteProfile::ArkCityFoundry: Family = TEXT("Civic"); break;
        case EDressingSiteProfile::CrownfallVoid: Family = TEXT("Choir"); break;
        case EDressingSiteProfile::SolarFallDais: Family = TEXT("Solar"); break;
        default: break;
    }
    const FString Prefix = FString::Printf(TEXT("/Game/Art/Generated/World/Environment/SM_World_%s"), Family);
    BiomeFormationMesh = LoadObject<UStaticMesh>(nullptr, *(Prefix + TEXT("Formation")));
    BiomeGroundMesh = LoadObject<UStaticMesh>(nullptr, *(Prefix + TEXT("Ground")));
    if (!BiomeFormationMesh || !BiomeGroundMesh)
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_WORLD_KIT_REFUSED] family=%s reason=missingRegisteredMesh"), Family);
        return false;
    }
    BlockedTiles->SetStaticMesh(BiomeFormationMesh);
    BiomeGround->SetStaticMesh(BiomeGroundMesh);
    UStaticMesh* SurfaceMesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_WalkSurface.SM_World_WalkSurface"));
    if (!SurfaceMesh) { return false; }
    BiomeSurface->SetStaticMesh(SurfaceMesh);
    M01ExteriorSkirt->ClearInstances();
    M01ExteriorSkirt->SetStaticMesh(SurfaceMesh);
    M01RouteActors.Reset();
    if (ActiveOperationMode == EEchoesOperationMode::CampaignPrologue)
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
            if (EchoesBattlefieldPresentation::IsGlassScarRoute(It->Tags)) M01RouteActors.Add(*It);
    M01ExteriorBanks->ClearAllMeshSections();
    BiomeHorizon->ClearInstances();
    BiomeHorizon->SetStaticMesh(BiomeFormationMesh);
    // Fixed distant scenery lies entirely outside the playable rectangle. It
    // describes the public setting, never hidden terrain or live game state.
    const float Radius = FMath::Max(MapWidthTiles, MapHeightTiles) * WorldUnitsPerTile * 1.1f;
    for (int32 Index = 0; ActiveOperationMode != EEchoesOperationMode::CampaignPrologue && Index < 20; ++Index)
    {
        const float Angle = Index * 2.0f * PI / 20.0f;
        const float Width = 20.0f + (Index * 7 % 5);
        const float Height = 6.0f + (Index * 11 % 7);
        BiomeHorizon->AddInstance(FTransform(FRotator(0, Index * 47.0f, 0),
            FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, -800),
            FVector(Width, Width * .72f, Height)));
    }
    // Public perimeter scenery closes the visible world edge without inventing
    // obstacles inside playable cells. M01's lower broken feet approach the
    // boundary without crossing it; other sites retain their prior clearance.
    const float HalfX = MapWidthTiles * WorldUnitsPerTile * .5f;
    const float HalfY = MapHeightTiles * WorldUnitsPerTile * .5f;
    const bool bM01 = ActiveOperationMode == EEchoesOperationMode::CampaignPrologue;
    if (bM01)
    {
        // Four abutting strips support the public basalt banks. They never
        // overlap a playable cell or participate in the scoped tile cache.
        const float MinX = -HalfX - WorldUnitsPerTile * .5f;
        const float MaxX = HalfX - WorldUnitsPerTile * .5f;
        const float MinY = -HalfY - WorldUnitsPerTile * .5f;
        const float MaxY = HalfY - WorldUnitsPerTile * .5f;
        // Continue the public substrate beneath the distant pressure field.
        // The normal6200 corner sweep qualifies this current extent; the
        // earlier rolled diagnostic does not establish a prior50m defect.
        constexpr float Depth = 10000.0f;
        const auto AddStrip = [this](float X0, float Y0, float X1, float Y1)
        {
            M01ExteriorSkirt->AddInstance(FTransform(FRotator::ZeroRotator,
                FVector((X0+X1)*.5f, (Y0+Y1)*.5f, 0),
                FVector((X1-X0)/200.0f, (Y1-Y0)/200.0f, 1)));
        };
        AddStrip(MinX-Depth, MinY-Depth, MaxX+Depth, MinY);
        AddStrip(MinX-Depth, MaxY, MaxX+Depth, MaxY+Depth);
        AddStrip(MinX-Depth, MinY, MinX, MaxY);
        AddStrip(MaxX, MinY, MaxX+Depth, MaxY);
    }
    const bool bCavern = ActiveDressingProfile == EDressingSiteProfile::SubterraneanCaverns;
    const bool bCivic = ActiveDressingProfile == EDressingSiteProfile::LumeReach ||
        ActiveDressingProfile == EDressingSiteProfile::ArkCityFoundry;
    for (int32 Side = 0; Side < 4; ++Side)
    {
        const float AlongHalf = Side % 2 == 0 ? HalfX : HalfY;
        // M01 uses joined public basalt slopes, built below with its material.
        if (bM01) continue;
        const int32 Count = FMath::CeilToInt(AlongHalf * 2 / 900.0f);
        for (int32 Index = 0; Index <= Count; ++Index)
        {
            const float Along = -AlongHalf + Index * (2 * AlongHalf / Count);
            const float Width = 6.0f + (Index * 3 + Side) % 3;
            const float Height = bCavern ? 8.0f + Index % 4 : bCivic ? 3.0f + Index % 3 : 2.5f + (Index % 4) * .5f;
            const FVector Center = Side == 0 ? FVector(Along, -HalfY - 1500, -180) :
                Side == 1 ? FVector(HalfX + 1500, Along, -180) :
                Side == 2 ? FVector(Along, HalfY + 1500, -180) : FVector(-HalfX - 1500, Along, -180);
            BiomeHorizon->AddInstance(FTransform(FRotator(0, bCivic ? Side * 90.0f : Index * 47.0f, 0),
                Center, FVector(Width, Width * .75f, Height)));
        }
    }
    FLinearColor Palette[] = {
        FLinearColor(.020f,.022f,.026f), FLinearColor(.008f,.010f,.013f),
        FLinearColor(.065f,.060f,.052f), FLinearColor(.50f,.03f,.34f)};
    if (ActiveDressingProfile == EDressingSiteProfile::LumeReach ||
        ActiveDressingProfile == EDressingSiteProfile::ArkCityFoundry)
    {
        Palette[0] = FLinearColor(.025f,.027f,.030f);
        Palette[2] = FLinearColor(.32f,.31f,.28f);
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::ShivergrassBasin)
    {
        Palette[2] = FLinearColor(.52f,.50f,.53f);
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::CrownfallVoid ||
             ActiveDressingProfile == EDressingSiteProfile::SolarFallDais)
    {
        Palette[3] = FLinearColor(.50f,.21f,.425f);
    }
    if (ActiveMapPreset == EEchoesSkirmishMapPreset::SorynConfluence)
    {
        Palette[3] = FLinearColor(.50f,.15f,.24f);
    }
    const TCHAR* TextureFamily = TEXT("T_EchoesGlassScarGround");
    if (ActiveDressingProfile == EDressingSiteProfile::ShivergrassBasin ||
        ActiveDressingProfile == EDressingSiteProfile::SubterraneanCaverns)
    {
        TextureFamily = TEXT("T_EchoesCausewayAsh");
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::LumeReach ||
             ActiveDressingProfile == EDressingSiteProfile::ArkCityFoundry ||
             ActiveDressingProfile == EDressingSiteProfile::SolarFallDais)
    {
        TextureFamily = TEXT("T_EchoesCeramicCivic");
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::CrownfallVoid)
    {
        TextureFamily = TEXT("T_EchoesVergeScored");
    }
    UTexture2D* SiteTextures[3] = {};
    const TCHAR* Suffixes[] = {TEXT("BaseColor"),TEXT("MRE"),TEXT("Normal")};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        SiteTextures[Index] = LoadObject<UTexture2D>(nullptr,
            *FString::Printf(TEXT("/Game/Art/Generated/Textures/%s_%s"),TextureFamily,Suffixes[Index]));
        if (!SiteTextures[Index]) { return false; }
    }
    for (int32 Slot = 0; Slot < 4; ++Slot)
    {
        BlockedMaterials[Slot]->SetVectorParameterValue(TerrainColorParameterName, Palette[Slot]);
        ScarredMaterials[Slot]->SetVectorParameterValue(TerrainColorParameterName, Palette[Slot]);
        BiomeGround->SetMaterial(Slot, ScarredMaterials[Slot]);
        BiomeSurface->SetMaterial(Slot, ScarredMaterials[Slot]);
        BiomeHorizon->SetMaterial(Slot, BlockedMaterials[Slot]);
        M01ExteriorSkirt->SetMaterial(Slot, ScarredMaterials[Slot]);
        for (UMaterialInstanceDynamic* Material : {BlockedMaterials[Slot].Get(), ScarredMaterials[Slot].Get()})
        {
            Material->SetScalarParameterValue(TEXT("WorldUVScale"),
                ActiveOperationMode == EEchoesOperationMode::CampaignPrologue ? .0004f : .0012f);
            Material->SetTextureParameterValue(TEXT("GroundBaseColorMap"),SiteTextures[0]);
            Material->SetTextureParameterValue(TEXT("GroundMREMap"),SiteTextures[1]);
            Material->SetTextureParameterValue(TEXT("GroundNormalMap"),SiteTextures[2]);
            Material->SetScalarParameterValue(TEXT("GroundGlowStrength"),
                ActiveDressingProfile == EDressingSiteProfile::GlassScar ? .25f : 0.0f);
        }
    }
    if (bM01)
    {
        UMaterialInterface* CliffMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/Generated/Materials/M_EchoesCliffSurface.M_EchoesCliffSurface"));
        if (!CliffMaterial)
        {
            UE_LOG(LogEchoes, Error, TEXT("[ECHOES_WORLD_KIT_REFUSED] family=Basalt reason=missingRegisteredCliffMaterial"));
            return false;
        }
        // Vertical public basalt uses the registered stratified cliff surface.
        // The horizontal backing retains the site's ground texture/material.
        for (int32 Slot = 0; Slot < 4; ++Slot) BiomeHorizon->SetMaterial(Slot, CliffMaterial);
        for (int32 Side = 0; Side < 4; ++Side)
        {
            const EchoesCliffMesh::FGeometry Geometry = EchoesCliffMesh::BuildExteriorBank(
                MapWidthTiles, MapHeightTiles, WorldUnitsPerTile, Side);
            M01ExteriorBanks->CreateMeshSection(Side, Geometry.Vertices, Geometry.Triangles,
                Geometry.Normals, Geometry.UV0, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
            M01ExteriorBanks->SetMaterial(Side, CliffMaterial);
        }
    }
    if (ActiveDressingProfile == EDressingSiteProfile::ShivergrassBasin)
    {
        UMaterialInterface* LeafParent = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/Generated/Materials/M_EchoesShivergrassLeaf.M_EchoesShivergrassLeaf"));
        if (!LeafParent) return false;
        UMaterialInstanceDynamic* Leaves = UMaterialInstanceDynamic::Create(LeafParent, this);
        if (!Leaves) return false;
        Leaves->SetVectorParameterValue(TerrainColorParameterName, Palette[2]);
        BiomeGround->SetMaterial(2, Leaves);
    }
    UE_LOG(LogEchoes, Display,
        TEXT("[ECHOES_WORLD_KIT_READY] family=%s revision=soryn-world-kits-v9 tileDetailsFogScoped=true horizonPublic=true collision=false navigation=false shadows=false"), Family);
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

    if (ActiveOperationMode.has_value())
    {
        switch (*ActiveOperationMode)
        {
            case EEchoesOperationMode::CampaignPrologue:
                ActiveDressingProfile = EDressingSiteProfile::GlassScar;
                break;
            case EEchoesOperationMode::CampaignSevenAccounts:
            case EEchoesOperationMode::CampaignShapeOfSilence:
                ActiveDressingProfile = EDressingSiteProfile::ShivergrassBasin;
                break;
            case EEchoesOperationMode::CampaignCityReserve:
            case EEchoesOperationMode::CampaignNamesWithoutBirths:
                ActiveDressingProfile = EDressingSiteProfile::ArkCityFoundry;
                break;
            case EEchoesOperationMode::CampaignUnburiedRoad:
            case EEchoesOperationMode::CampaignFutureThatWon:
                ActiveDressingProfile = EDressingSiteProfile::SubterraneanCaverns;
                break;
            case EEchoesOperationMode::CampaignTermsOfContinuance:
            case EEchoesOperationMode::CampaignShapeBesideUs:
            case EEchoesOperationMode::CampaignReserveAuthority:
            case EEchoesOperationMode::CampaignNoNeutralLedger:
                ActiveDressingProfile = EDressingSiteProfile::CrownfallVoid;
                break;
            case EEchoesOperationMode::CampaignChoirAtLumeReach:
                ActiveDressingProfile = EDressingSiteProfile::LumeReach;
                break;
            case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
            case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
            case EEchoesOperationMode::CampaignTheBrokenSun:
                ActiveDressingProfile = EDressingSiteProfile::SolarFallDais;
                break;
            case EEchoesOperationMode::Skirmish:
            default:
                if (ActiveMapPreset == EEchoesSkirmishMapPreset::GlassScar)
                {
                    ActiveDressingProfile = EDressingSiteProfile::GlassScar;
                }
                else if (ActiveMapPreset == EEchoesSkirmishMapPreset::CrownfallBasin)
                {
                    ActiveDressingProfile = EDressingSiteProfile::CrownfallVoid;
                }
                else if (ActiveMapPreset == EEchoesSkirmishMapPreset::SorynConfluence)
                {
                    ActiveDressingProfile = EDressingSiteProfile::GlassScar;
                }
                else
                {
                    ActiveDressingProfile = EDressingSiteProfile::None;
                }
                break;
        }
    }
    else
    {
        // Auto-detect topology if operation mode is not explicitly specified
        const int32 LumeReachCheckIndex = 29 * MapWidthTiles + 9;
        if (CachedTerrain.IsValidIndex(LumeReachCheckIndex) &&
            (CachedTerrain[LumeReachCheckIndex] & 0x03) == static_cast<uint8>(echoes::sim::Terrain::Blocked))
        {
            ActiveDressingProfile = EDressingSiteProfile::LumeReach;
        }
        else if (ActiveMapPreset == EEchoesSkirmishMapPreset::GlassScar)
        {
            ActiveDressingProfile = EDressingSiteProfile::GlassScar;
        }
        else if (ActiveMapPreset == EEchoesSkirmishMapPreset::CrownfallBasin)
        {
            ActiveDressingProfile = EDressingSiteProfile::CrownfallVoid;
        }
        else if (ActiveMapPreset == EEchoesSkirmishMapPreset::SorynConfluence)
        {
            ActiveDressingProfile = EDressingSiteProfile::GlassScar;
        }
        else
        {
            ActiveDressingProfile = EDressingSiteProfile::None;
        }
    }

    if (!ConfigureWorldKit() || !InitializeMissionLandmarks())
    {
        return false;
    }

    if (ActiveDressingProfile == EDressingSiteProfile::None)
    {
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
    else if (ActiveDressingProfile == EDressingSiteProfile::ShivergrassBasin)
    {
        ActiveDressingSiteId = TEXT("shivergrass-basin");
        ActiveDressingPackSha = TEXT("pending-authoring");
        ActiveDressingBasePackSha = TEXT("pending-authoring");
        DressingRecordCount = 0;
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::SubterraneanCaverns)
    {
        ActiveDressingSiteId = TEXT("subterranean-caverns");
        ActiveDressingPackSha = TEXT("pending-authoring");
        ActiveDressingBasePackSha = TEXT("pending-authoring");
        DressingRecordCount = 0;
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::ArkCityFoundry)
    {
        ActiveDressingSiteId = TEXT("arkcity-foundry");
        ActiveDressingPackSha = TEXT("pending-authoring");
        ActiveDressingBasePackSha = TEXT("pending-authoring");
        DressingRecordCount = 0;
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::CrownfallVoid)
    {
        ActiveDressingSiteId = TEXT("crownfall-void");
        ActiveDressingPackSha = TEXT("pending-authoring");
        ActiveDressingBasePackSha = TEXT("pending-authoring");
        DressingRecordCount = 0;
    }
    else if (ActiveDressingProfile == EDressingSiteProfile::SolarFallDais)
    {
        ActiveDressingSiteId = TEXT("solar-fall-dais");
        ActiveDressingPackSha = TEXT("pending-authoring");
        ActiveDressingBasePackSha = TEXT("pending-authoring");
        DressingRecordCount = 0;
    }

    if (DressingRecordCount == 0)
    {
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_SITE_LANDMARKS_PENDING] site=%s tileKit=ready authoredLandmarks=false"),
            *ActiveDressingSiteId);
        return true;
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

bool AEchoesTerrainView::InitializeMissionLandmarks()
{
    bMissionLandmarksActive = false;
    MissionLandmarkInstanceIndices.Reset();
    MissionLandmarkDrawn.Reset();
    MissionLandmarkSolidCells.Reset();
    MissionLandmarkPavingCells.Reset();
    CliffDrawMask.Reset();
    ContinuousCliffs->ClearAllMeshSections();
    for (UInstancedStaticMeshComponent* Layer : MissionLandmarkLayers) Layer->ClearInstances();
    const mission_landmarks::Pack* Pack = ActiveMissionLandmarkPack(ActiveOperationMode);
    if (Pack == nullptr) return true;
    // Records are compiled only where every doctrine agrees. Preserve verifies
    // source identity; live scoped terrain still controls every draw decision.
    const auto Map = echoes::world::CheckCampaignTerrain(
        Pack->mission_ordinal, echoes::sim::FutureWellChoice::Preserve);
    if (!Map.ok || MapWidthTiles != 64 || MapHeightTiles != 64 ||
        std::string_view(Map.map_id) != Pack->map_id ||
        std::string_view(Map.source_sha256) != Pack->terrain_source_sha256)
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_MISSION_LANDMARKS_REFUSED] reason=terrainSourceMismatch"));
        return false;
    }
    if (Pack->kind_count > static_cast<size_t>(MissionLandmarkLayers.Num()))
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_MISSION_LANDMARKS_REFUSED] reason=layerCapacity"));
        return false;
    }
    for (size_t Kind = 0; Kind < Pack->kind_count; ++Kind)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/Art/Generated/World/Environment/SM_World_%s%s"),
                UTF8_TO_TCHAR(Pack->mission_code.data()), UTF8_TO_TCHAR(Pack->kind_names[Kind])));
        if (!Mesh)
        {
            UE_LOG(LogEchoes, Error, TEXT("[ECHOES_MISSION_LANDMARKS_REFUSED] reason=missingRegisteredMesh kind=%s"),
                UTF8_TO_TCHAR(Pack->kind_names[Kind]));
            return false;
        }
        MissionLandmarkLayers[Kind]->SetStaticMesh(Mesh);
    }
    for (size_t Index = 0; Index < Pack->record_count; ++Index)
    {
        const auto& Record = Pack->records[Index];
        if (Record.kind >= MissionLandmarkLayers.Num() || Record.x >= MapWidthTiles || Record.y >= MapHeightTiles ||
            Record.footprint_x0 > Record.footprint_x1 || Record.footprint_y0 > Record.footprint_y1 ||
            Record.footprint_x1 >= MapWidthTiles || Record.footprint_y1 >= MapHeightTiles) return false;
        MissionLandmarkInstanceIndices.Add(MissionLandmarkLayers[Record.kind]->AddInstance(HiddenTransform(), false));
        MissionLandmarkDrawn.Add(255);
        for (int32 Y = Record.footprint_y0; Y <= Record.footprint_y1; ++Y)
            for (int32 X = Record.footprint_x0; X <= Record.footprint_x1; ++X)
            {
                if (Record.requires_blocked) MissionLandmarkSolidCells.Add(Y * MapWidthTiles + X);
                else MissionLandmarkPavingCells.Add(Y * MapWidthTiles + X);
            }
    }
    UMaterialInterface* CliffMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/Generated/Materials/M_EchoesCliffSurface"));
    if (CliffMaterial == nullptr)
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_MISSION_LANDMARKS_REFUSED] reason=missingCliffMaterial"));
        return false;
    }
    ContinuousCliffs->SetMaterial(0, CliffMaterial);
    bMissionLandmarksActive = true;
    UE_LOG(LogEchoes, Display,
        TEXT("[ECHOES_MISSION_LANDMARKS_READY] mission=%s records=%d source=%s fogScoped=true collision=false navigation=false finalArt=false"),
        UTF8_TO_TCHAR(Pack->mission_code.data()), MissionLandmarkInstanceIndices.Num(),
        UTF8_TO_TCHAR(Pack->source_sha256.data()));
    return true;
}

void AEchoesTerrainView::SyncMissionLandmarks(
    TFunctionRef<echoes::sim::Terrain(int32, int32)> TerrainAt,
    TFunctionRef<echoes::sim::Visibility(int32, int32)> VisibilityAt)
{
    if (!bMissionLandmarksActive) return;
    const mission_landmarks::Pack* Pack = ActiveMissionLandmarkPack(ActiveOperationMode);
    if (Pack == nullptr || Pack->record_count != static_cast<size_t>(MissionLandmarkInstanceIndices.Num())) return;
    TArray<bool> Changed;
    Changed.Init(false, MissionLandmarkLayers.Num());
    for (int32 Index = 0; Index < MissionLandmarkInstanceIndices.Num(); ++Index)
    {
        const auto& Record = Pack->records[Index];
        bool Draw = true;
        for (int32 Y = Record.footprint_y0; Draw && Y <= Record.footprint_y1; ++Y)
            for (int32 X = Record.footprint_x0; Draw && X <= Record.footprint_x1; ++X)
            {
                // A shared apron is shown only after its whole footprint is known.
                // Never fetch undisclosed terrain to decide its appearance.
                Draw = VisibilityAt(X, Y) != echoes::sim::Visibility::Unexplored;
                if (Draw) Draw = (TerrainAt(X, Y) == echoes::sim::Terrain::Blocked) == Record.requires_blocked;
            }
        if (MissionLandmarkDrawn[Index] == static_cast<uint8>(Draw)) continue;
        const float Z = Pack->mission_ordinal == 1
            ? (Record.kind == 4 ? 0.0f : 1.0f)
            : (Record.requires_blocked ? 1.0f : 0.0f);
        const FVector Location((Record.x + Record.pivot_x_half_tiles * .5f - MapWidthTiles * .5f) * WorldUnitsPerTile,
                               (Record.y + Record.pivot_y_half_tiles * .5f - MapHeightTiles * .5f) * WorldUnitsPerTile, Z);
        MissionLandmarkLayers[Record.kind]->UpdateInstanceTransform(MissionLandmarkInstanceIndices[Index],
            Draw ? FTransform(FRotator(0, Record.yaw, 0), Location, FVector(WorldUnitsPerTile / 200.0f)) : HiddenTransform(),
            false, false, true);
        MissionLandmarkDrawn[Index] = static_cast<uint8>(Draw);
        Changed[Record.kind] = true;
    }
    for (int32 Kind = 0; Kind < MissionLandmarkLayers.Num(); ++Kind)
        if (Changed[Kind]) MissionLandmarkLayers[Kind]->MarkRenderStateDirty();
}

void AEchoesTerrainView::SyncContinuousCliffs(
    TFunctionRef<echoes::sim::Terrain(int32, int32)> TerrainAt,
    TFunctionRef<echoes::sim::Visibility(int32, int32)> VisibilityAt,
    TFunctionRef<bool(int32, int32)> IsPassable)
{
    const mission_landmarks::Pack* Pack = ActiveMissionLandmarkPack(ActiveOperationMode);
    // The continuous cliff compositor is Glass Scar-specific. M02 retains its
    // own Shivergrass world kit and only adds its fog-scoped landmark meshes.
    if (!bMissionLandmarksActive || Pack == nullptr || Pack->mission_ordinal != 1 ||
        BlockedMaterials.IsEmpty()) return;
    const int32 Count = MapWidthTiles * MapHeightTiles;
    constexpr int32 ChunkSize = 8;
    const int32 Columns = FMath::DivideAndRoundUp(MapWidthTiles, ChunkSize);
    TArray<uint8> NextMask;
    NextMask.SetNumZeroed(Count);
    TSet<int32> DirtyChunks;
    const auto MarkChunk = [&](int32 X, int32 Y)
    {
        if (X >= 0 && Y >= 0 && X < MapWidthTiles && Y < MapHeightTiles)
            DirtyChunks.Add((Y / ChunkSize) * Columns + X / ChunkSize);
    };
    for (int32 Y = 0; Y < MapHeightTiles; ++Y)
        for (int32 X = 0; X < MapWidthTiles; ++X)
        {
            const int32 Index = Y * MapWidthTiles + X;
            const bool bInChasm = bChasmComposed && Y >= ChasmBandMinRow && Y <= ChasmBandMaxRow;
            NextMask[Index] = VisibilityAt(X,Y) != echoes::sim::Visibility::Unexplored &&
                TerrainAt(X,Y) == echoes::sim::Terrain::Blocked && !bInChasm &&
                !MissionLandmarkSolidCells.Contains(Index) && !IsPassable(X,Y);
            if (!CliffDrawMask.IsValidIndex(Index) || CliffDrawMask[Index] != NextMask[Index])
            {
                MarkChunk(X,Y);
                // Corner fracture samples also depend on diagonal known cells.
                for (int32 DY = -1; DY <= 1; ++DY)
                    for (int32 DX = -1; DX <= 1; ++DX) MarkChunk(X+DX,Y+DY);
            }
        }
    // Sorted section updates avoid a platform-dependent iteration order even though
    // the mesh is disposable presentation and never enters a simulation checksum.
    TArray<int32> Sections = DirtyChunks.Array();
    Sections.Sort();
    for (const int32 Section : Sections)
    {
        const int32 MinX = (Section % Columns) * ChunkSize;
        const int32 MinY = (Section / Columns) * ChunkSize;
        const EchoesCliffMesh::FGeometry Geometry = EchoesCliffMesh::BuildChunk(
            MapWidthTiles, MapHeightTiles, WorldUnitsPerTile, NextMask, MinX, MinY,
            FMath::Min(MinX + ChunkSize, MapWidthTiles), FMath::Min(MinY + ChunkSize, MapHeightTiles));
        if (Geometry.Vertices.IsEmpty()) ContinuousCliffs->ClearMeshSection(Section);
        else
        {
            TArray<FProcMeshTangent> Tangents;
            Tangents.Reserve(Geometry.Normals.Num());
            for (const FVector& Normal : Geometry.Normals)
            {
                const FVector Axis = FMath::Abs(Normal.X) > .8f ? FVector::YAxisVector : FVector::XAxisVector;
                Tangents.Emplace((Axis - Normal * FVector::DotProduct(Axis, Normal)).GetSafeNormal(), false);
            }
            ContinuousCliffs->CreateMeshSection(Section, Geometry.Vertices, Geometry.Triangles,
                Geometry.Normals, Geometry.UV0, TArray<FColor>(), Tangents, false);
            ContinuousCliffs->SetMaterial(Section, ContinuousCliffs->GetMaterial(0));
        }
    }
    CliffDrawMask = MoveTemp(NextMask);
}

void AEchoesTerrainView::SyncDressingWith(
    TFunctionRef<echoes::sim::Terrain(int32, int32)> TerrainAt,
    TFunctionRef<echoes::sim::Visibility(int32, int32)> VisibilityAt,
    TFunctionRef<bool(int32, int32)> ReshapedOpenAt)
{
    SyncMissionLandmarks(TerrainAt, VisibilityAt);
    SyncContinuousCliffs(TerrainAt, VisibilityAt, ReshapedOpenAt);
    if (!bDressingActive)
    {
        return;
    }
    // Theme names are not topology identities. In a scoped view, the Blocked
    // value on an unexplored cell is a sentinel, not evidence that a dressing
    // record belongs there. Admit static dressing only where the authored map
    // supports it across its doctrine variants. Legacy diagnostic views may
    // still test an explicitly supplied unscoped terrain fixture.
    if (bDressingAwaitingIdentity)
    {
        bDressingAwaitingIdentity = false;
        int32 Mismatched = 0;
        for (int32 RecordIndex = 0; RecordIndex < DressingRecordCount; ++RecordIndex)
        {
            const FActiveDressingRecord& Record = ActiveDressingRecords[RecordIndex];
            bool bAuthoredBlocked = false;
            if (bDiagnosticDressingTopology)
                bAuthoredBlocked = TerrainAt(Record.X, Record.Y) == echoes::sim::Terrain::Blocked;
            else
            {
                EEchoesCampaignMissionId Mission;
                if (ActiveOperationMode.has_value() &&
                    UEchoesSimulationSubsystem::GetMissionIdForOperation(*ActiveOperationMode, Mission))
                {
                    const int32 Ordinal = static_cast<int32>(Mission);
                    bAuthoredBlocked = true;
                    for (const auto Choice : {echoes::sim::FutureWellChoice::Harvest,
                            echoes::sim::FutureWellChoice::Preserve, echoes::sim::FutureWellChoice::Reshape})
                        bAuthoredBlocked &= !echoes::world::IsCampaignTerrainPassable(
                            Ordinal, Choice, Record.X, Record.Y);
                }
                else
                    bAuthoredBlocked = FEchoesSkirmishSetupModel::IsBlockedTile(
                        ActiveMapPreset, Record.X, Record.Y);
            }
            if (!bAuthoredBlocked)
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
                TEXT("[ECHOES_DRESSING_INACTIVE] site=%s records=%d offBlocked=%d reason=topologyDoesNotSupportBoundDressing"),
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
            // Observing a changed cell is normal player knowledge, not an
            // asset-binding failure. The authored check above is independent.
            if (bDiagnosticDressingTopology && !DressingRefusalReported[RecordIndex])
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
            TerrainMetallicParameterName, 0.03f);
        Material->SetScalarParameterValue(
            TerrainRoughnessParameterName, 0.94f);
        Material->SetScalarParameterValue(
            TerrainEmissiveParameterName, MaterialIndex == 3 ? 0.12f : 0.0f);
        Materials.Add(Material);
        Layer->SetMaterial(MaterialIndex, Material);
    }
}

void AEchoesTerrainView::ClearChasmComposition()
{
    bChasmComposed = false;
    ChasmInstances.Reset();
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

bool AEchoesTerrainView::IsWorldBoundsKnown(const FBox& Bounds) const
{
    if (!Bounds.IsValid || WorldUnitsPerTile <= 0 ||
        CachedTerrain.Num() != MapWidthTiles * MapHeightTiles) return false;
    const int32 MinX = FMath::Clamp(FMath::FloorToInt(Bounds.Min.X / WorldUnitsPerTile + MapWidthTiles * .5f + .5f), 0, MapWidthTiles - 1);
    const int32 MaxX = FMath::Clamp(FMath::CeilToInt(Bounds.Max.X / WorldUnitsPerTile + MapWidthTiles * .5f + .5f) - 1, 0, MapWidthTiles - 1);
    const int32 MinY = FMath::Clamp(FMath::FloorToInt(Bounds.Min.Y / WorldUnitsPerTile + MapHeightTiles * .5f + .5f), 0, MapHeightTiles - 1);
    const int32 MaxY = FMath::Clamp(FMath::CeilToInt(Bounds.Max.Y / WorldUnitsPerTile + MapHeightTiles * .5f + .5f) - 1, 0, MapHeightTiles - 1);
    for (int32 Y = MinY; Y <= MaxY; ++Y)
        for (int32 X = MinX; X <= MaxX; ++X)
        {
            const uint8 State = CachedTerrain[Y * MapWidthTiles + X];
            if (State == 255 || (State >> 2) == static_cast<uint8>(echoes::sim::Visibility::Unexplored)) return false;
        }
    return true;
}

void AEchoesTerrainView::SyncChasmVisibility()
{
    for (const TWeakObjectPtr<AActor>& Route : M01RouteActors)
        if (Route.IsValid()) Route->SetActorHiddenInGame(!IsWorldBoundsKnown(Route->GetComponentsBoundingBox(true)));

    if (!bChasmComposed) return;
    // A span discloses both its footprint and the local crossing classification.
    // Require all of that terrain to be known, including the adjacent band rows.
    // Remembered (explored) terrain remains legal; reset/unexplored hides it again.
    const auto KnownBounds = [this](const FBox& Bounds)
    {
        const int32 MinX = FMath::Clamp(FMath::FloorToInt(Bounds.Min.X / WorldUnitsPerTile + MapWidthTiles * .5f - .5f), 0, MapWidthTiles - 1);
        const int32 MaxX = FMath::Clamp(FMath::CeilToInt(Bounds.Max.X / WorldUnitsPerTile + MapWidthTiles * .5f + .5f), 0, MapWidthTiles - 1);
        const int32 MinY = FMath::Clamp(FMath::Min(ChasmBandMinRow - 1, FMath::FloorToInt(Bounds.Min.Y / WorldUnitsPerTile + MapHeightTiles * .5f - .5f)), 0, MapHeightTiles - 1);
        const int32 MaxY = FMath::Clamp(FMath::Max(ChasmBandMaxRow + 1, FMath::CeilToInt(Bounds.Max.Y / WorldUnitsPerTile + MapHeightTiles * .5f + .5f)), 0, MapHeightTiles - 1);
        for (int32 Y = MinY; Y <= MaxY; ++Y)
            for (int32 X = MinX; X <= MaxX; ++X)
            {
                const uint8 State = CachedTerrain[Y * MapWidthTiles + X];
                if (State == 255 || (State >> 2) == static_cast<uint8>(echoes::sim::Visibility::Unexplored)) return false;
            }
        return true;
    };
    for (FChasmInstance& Instance : ChasmInstances)
    {
        const FBox Bounds = Instance.Layer->GetStaticMesh()->GetBoundingBox().TransformBy(Instance.Transform);
        const bool bVisible = ActiveOperationMode == EEchoesOperationMode::CampaignPrologue &&
            Instance.Layer == ChasmBed ? IsWorldBoundsKnown(Bounds) : KnownBounds(Bounds);
        if (bVisible != Instance.bVisible)
        {
            Instance.Layer->UpdateInstanceTransform(Instance.Index, bVisible ? Instance.Transform : HiddenTransform(), false, true, true);
            Instance.bVisible = bVisible;
        }
    }
    for (UPointLightComponent* Light : ChasmFissureLights)
    {
        const FVector Center = Light->GetRelativeLocation();
        const FVector Extent(Light->AttenuationRadius);
        Light->SetVisibility(KnownBounds(FBox(Center - Extent, Center + Extent)));
    }
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
    ContinuousCliffs->SetVisibility(bVisible);
    for (UInstancedStaticMeshComponent* Layer : MissionLandmarkLayers) Layer->SetVisibility(bVisible);

    for (UInstancedStaticMeshComponent* Layer :
         {BlockedTiles, ScarredTiles, BiomeGround, BiomeSurface, DressingShelves, DressingShards})
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
    const bool bM01 = ActiveOperationMode == EEchoesOperationMode::CampaignPrologue;
    const FLinearColor BankColor(0.008f, 0.010f, 0.013f);
    const FLinearColor TerraceColor(0.006f, 0.009f, 0.012f);
    const FLinearColor BedColor(0.030f, 0.026f, 0.024f);
    const FLinearColor FissureGlow(1.0f, 0.42f, 0.08f);
    ConfigureChasmLayer(ChasmBanks, ScarredMesh, BankColor, FissureGlow, ChasmBankMaterials);
    ConfigureChasmLayer(ChasmTerrace, ScarredMesh, TerraceColor, FissureGlow, ChasmBankMaterials);
    ConfigureChasmLayer(ChasmTeeth, BiomeFormationMesh, FLinearColor(0.030f, 0.040f, 0.048f), FissureGlow, ChasmTeethMaterials);
    ConfigureChasmLayer(ChasmBed, bM01 ? BiomeSurface->GetStaticMesh() : ScarredMesh,
        BedColor, FissureGlow, ChasmBedMaterials);
    if (bM01)
    {
        // The sealed bed and cut faces are basalt. Horizontal world-XY ground
        // maps extrude their veins down vertical walls; the registered cliff
        // master uses 3D geology and real normals, with no emissive path.
        if (UMaterialInterface* Basalt = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/Generated/Materials/M_EchoesCliffSurface.M_EchoesCliffSurface")))
        {
            ChasmBedMaterials.Reset();
            for (int32 Slot = 0; Slot < 4; ++Slot)
                ChasmBed->SetMaterial(Slot, Basalt);
        }
        else
        {
            UE_LOG(LogEchoes, Error, TEXT("[ECHOES_M01_BASALT_DEGRADED] required registered cliff material unavailable; surface review cannot pass"));
        }
    }

    // 3. Banks: overlapping shelf plates from each rim out to the map edge.
    //    The shelf is 780 wide; plates overlap by ~60 and alternate 6 lower
    //    so seams do not show. Row 0 is deep enough to reach the bed.
    constexpr float ShelfSource = 780.0f;
    constexpr float PlateScaleXY = 2.05f;
    constexpr float PlateWidth = ShelfSource * PlateScaleXY;
    constexpr float PlateSpacing = PlateWidth - 60.0f;
    constexpr float PlateTopLocal = 39.0f;
    constexpr float PlateTopZ = -6.0f; // just under the deck of a crossing
    constexpr float RimRowScaleZ = 4.5f;
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
                const float Stagger = 0.0f; // one continuous top datum; no raised tile seams
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
    constexpr float TerraceScaleZ = 2.8f;
    constexpr float TerraceTopZ = -780.0f;
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
                FVector(CenterX, CenterY + 110.0f * FMath::Sin(Column * 1.71f),
                        TerraceTopZ - PlateTopLocal * TerraceScaleZ + 95.0f * FMath::Sin(Column * 2.13f)),
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
        if (bM01)
        {
            // Remembered bed cells must render during partial reconnaissance.
            // A single span-sized mesh stayed hidden until its far end was seen,
            // leaving a blue background hole in otherwise known terrain.
            if (Span.bDeep || Span.bTouchesEdge)
                for (float X = Span.MinX + T * .5f; X < Span.MaxX; X += T)
                    for (float Y = BandMinY + T * .5f; Y < BandMaxY; Y += T)
                        ChasmBed->AddInstance(FTransform(FRotator::ZeroRotator,
                            FVector(X, Y, Span.bDeep ? ChasmBedTopZ : PlateTopZ),
                            FVector(T / 200.0f, T / 200.0f, 1)));
        }
        else if (Span.bDeep)
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

    if (bM01)
    {
        // The shelf recipe's separate fracture wedges do not seal the bank.
        // Close the deep-span shell with the registered thin substrate behind
        // that dressing. Each 200-cm segment stays inside one blocked rim cell
        // and uses the bed layer's exact knowledge admission; routes stay open.
        const float WallBottom = ChasmBedTopZ - 8.0f;
        const float WallHeight = PlateTopZ - WallBottom;
        const float WallCenterZ = (PlateTopZ + WallBottom) * .5f;
        for (const FSpan& Span : Spans)
        {
            if (!Span.bDeep) continue;
            for (float X = Span.MinX + T * .5f; X < Span.MaxX; X += T)
            {
                ChasmBed->AddInstance(FTransform(FRotator(0, 0, 90),
                    FVector(X, BandMinY + 1, WallCenterZ), FVector(T / 200, WallHeight / 200, 1)));
                ChasmBed->AddInstance(FTransform(FRotator(0, 0, -90),
                    FVector(X, BandMaxY - 1, WallCenterZ), FVector(T / 200, WallHeight / 200, 1)));
            }
            for (float Y = BandMinY + T * .5f; Y < BandMaxY; Y += T)
            {
                ChasmBed->AddInstance(FTransform(FRotator(90, 0, 0),
                    FVector(Span.MinX + 3, Y, WallCenterZ), FVector(WallHeight / 200, T / 200, 1)));
                ChasmBed->AddInstance(FTransform(FRotator(-90, 0, 0),
                    FVector(Span.MaxX - 3, Y, WallCenterZ), FVector(WallHeight / 200, T / 200, 1)));
            }
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
            const float Scale = bM01
                ? 0.72f + 0.36f * (0.5f + 0.5f * FMath::Sin(Tooth * 2.17f))
                : 0.55f + 0.2f * static_cast<float>(Tooth % 3);
            ChasmTeeth->AddInstance(FTransform(
                FRotator(0.0f, 37.0f * static_cast<float>(Tooth) + (BankSign < 0.0f ? 0.0f : 180.0f), 0.0f),
                FVector(JitteredX, RimY, bM01 ? ChasmBedTopZ - 8.0f : -260.0f),
                FVector(Scale, Scale, bM01 ? Scale * 2.5f : Scale)));
        }
    }

    // M01 reuses low, cleaved basalt on the bed. Bounds stay inside deep spans;
    // the existing full-footprint knowledge check governs every fragment.
    if (bM01)
    {
        int32 Fragment = 0;
        for (const FSpan& Span : Spans)
        {
            if (!Span.bDeep) continue;
            const float Width = Span.MaxX - Span.MinX;
            const int32 Count = FMath::Max(1, FMath::FloorToInt(Width / 750.0f));
            const float Segment = Width / Count;
            for (int32 Index = 0; Index < Count; ++Index, ++Fragment)
            {
                const float ScaleX = FMath::Min(3.6f, Segment / 260.0f);
                const float ScaleY = FMath::Min(1.6f, BandHalfWidth / 230.0f);
                const float X = Span.MinX + Segment * (Index + .5f);
                const float Y = BandCenterY + 85.0f * FMath::Sin(Fragment * 1.83f);
                ChasmTeeth->AddInstance(FTransform(
                    FRotator(0, 8.0f * FMath::Sin(Fragment * 2.31f), 0),
                    FVector(X, Y, ChasmBedTopZ - 8.0f),
                    FVector(ScaleX, ScaleY, .6f + .32f * (Fragment % 3))));
            }
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
    for (UInstancedStaticMeshComponent* Layer :
         {ChasmBanks, ChasmTerrace, ChasmTeeth, ChasmBed})
    {
        for (int32 Index = 0; Index < Layer->GetInstanceCount(); ++Index)
        {
            FTransform Transform;
            Layer->GetInstanceTransform(Index, Transform);
            ChasmInstances.Add({Layer, Index, Transform, true});
        }
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
