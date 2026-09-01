#include "EchoesTerrainView.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesBattlefieldPresentation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName TerrainColorParameterName(TEXT("Color"));
const FName TerrainMetallicParameterName(TEXT("Metallic"));
const FName TerrainRoughnessParameterName(TEXT("Roughness"));
const FName TerrainEmissiveParameterName(TEXT("EmissiveStrength"));
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

    for (UInstancedStaticMeshComponent* Layer : {BlockedTiles, ScarredTiles})
    {
        Layer->SetMobility(EComponentMobility::Movable);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCastShadow(false);
        Layer->SetReceivesDecals(false);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RidgeFinder(
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarRidge.SM_World_GlassScarRidge"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShelfFinder(
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarShelf.SM_World_GlassScarShelf"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesWorldSurface.M_EchoesWorldSurface"));
    BlockedMesh = RidgeFinder.Object;
    ScarredMesh = ShelfFinder.Object;
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

bool AEchoesTerrainView::InitializeTerrain(
    const echoes::sim::Simulation& Simulation,
    float TileWorldSize,
    EEchoesSkirmishMapPreset MapPreset)
{
    if (!InitializeScopedTerrain(
            Simulation.Config().mapWidthTiles,
            Simulation.Config().mapHeightTiles,
            TileWorldSize,
            MapPreset))
    {
        return false;
    }
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
    const int32 TileCount = MapWidthTiles * MapHeightTiles;
    CachedTerrain.Init(255, TileCount);
    BlockedTiles->SetStaticMesh(BlockedMesh);
    ScarredTiles->SetStaticMesh(ScarredMesh);
    BlockedMaterials.Reset();
    ScarredMaterials.Reset();
    FLinearColor BlockedColors[] = {
        FLinearColor(0.075f, 0.045f, 0.050f),
        FLinearColor(0.010f, 0.014f, 0.020f),
        FLinearColor(0.24f, 0.18f, 0.15f),
        FLinearColor(0.58f, 0.035f, 0.18f)};
    FLinearColor ScarredColors[] = {
        FLinearColor(0.12f, 0.065f, 0.035f),
        FLinearColor(0.025f, 0.020f, 0.018f),
        FLinearColor(0.30f, 0.21f, 0.13f),
        FLinearColor(0.94f, 0.26f, 0.035f)};
    if (ActiveMapPreset == EEchoesSkirmishMapPreset::CrownfallBasin)
    {
        BlockedColors[0] = FLinearColor(0.055f, 0.090f, 0.105f);
        BlockedColors[1] = FLinearColor(0.012f, 0.025f, 0.030f);
        BlockedColors[2] = FLinearColor(0.26f, 0.22f, 0.12f);
        BlockedColors[3] = FLinearColor(0.88f, 0.48f, 0.08f);
        ScarredColors[0] = FLinearColor(0.070f, 0.11f, 0.085f);
        ScarredColors[1] = FLinearColor(0.018f, 0.030f, 0.025f);
        ScarredColors[2] = FLinearColor(0.31f, 0.24f, 0.11f);
        ScarredColors[3] = FLinearColor(0.78f, 0.60f, 0.15f);
    }
    else if (ActiveMapPreset == EEchoesSkirmishMapPreset::SorynConfluence)
    {
        BlockedColors[0] = FLinearColor(0.075f, 0.050f, 0.13f);
        BlockedColors[1] = FLinearColor(0.012f, 0.018f, 0.035f);
        BlockedColors[2] = FLinearColor(0.10f, 0.28f, 0.31f);
        BlockedColors[3] = FLinearColor(0.12f, 0.88f, 0.92f);
        ScarredColors[0] = FLinearColor(0.045f, 0.10f, 0.15f);
        ScarredColors[1] = FLinearColor(0.015f, 0.022f, 0.040f);
        ScarredColors[2] = FLinearColor(0.18f, 0.24f, 0.42f);
        ScarredColors[3] = FLinearColor(0.34f, 0.74f, 1.0f);
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
    return true;
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
    bool bChanged = false;
    const FTransform Hidden = HiddenTransform();
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            const int32 TileIndex = TileY * MapWidthTiles + TileX;
            const echoes::sim::Terrain Terrain =
                Tiles[static_cast<std::size_t>(TileIndex)].terrain;
            BlockedTileCount +=
                Terrain == echoes::sim::Terrain::Blocked ? 1 : 0;
            ScarredTileCount +=
                Terrain == echoes::sim::Terrain::Scarred ? 1 : 0;
            const uint8 Encoded = static_cast<uint8>(Terrain);
            if (CachedTerrain[TileIndex] == Encoded)
            {
                continue;
            }
            CachedTerrain[TileIndex] = Encoded;
            BlockedTiles->UpdateInstanceTransform(
                TileIndex,
                Terrain == echoes::sim::Terrain::Blocked
                    ? TileTransform(TileX, TileY, Terrain)
                    : Hidden,
                false,
                false,
                true);
            ScarredTiles->UpdateInstanceTransform(
                TileIndex,
                Terrain == echoes::sim::Terrain::Scarred
                    ? TileTransform(TileX, TileY, Terrain)
                    : Hidden,
                false,
                false,
                true);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        BlockedTiles->MarkRenderStateDirty();
        ScarredTiles->MarkRenderStateDirty();
    }
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
    bool bChanged = false;
    const FTransform Hidden = HiddenTransform();
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            const int32 TileIndex = TileY * MapWidthTiles + TileX;
            const echoes::sim::Terrain Terrain =
                Simulation.TerrainAt(TileX, TileY);
            BlockedTileCount += Terrain == echoes::sim::Terrain::Blocked ? 1 : 0;
            ScarredTileCount += Terrain == echoes::sim::Terrain::Scarred ? 1 : 0;
            const uint8 Encoded = static_cast<uint8>(Terrain);
            if (CachedTerrain[TileIndex] == Encoded)
            {
                continue;
            }
            CachedTerrain[TileIndex] = Encoded;
            BlockedTiles->UpdateInstanceTransform(
                TileIndex,
                Terrain == echoes::sim::Terrain::Blocked
                    ? TileTransform(TileX, TileY, Terrain)
                    : Hidden,
                false,
                false,
                true);
            ScarredTiles->UpdateInstanceTransform(
                TileIndex,
                Terrain == echoes::sim::Terrain::Scarred
                    ? TileTransform(TileX, TileY, Terrain)
                    : Hidden,
                false,
                false,
                true);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        BlockedTiles->MarkRenderStateDirty();
        ScarredTiles->MarkRenderStateDirty();
    }
    return true;
}
