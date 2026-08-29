#include "EchoesTerrainView.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName ColorParameterName(TEXT("Color"));
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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = CubeFinder.Object;
    BasicMaterial = MaterialFinder.Object;
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
    const float Height = bBlocked ? 8.0f : 2.0f;
    const float Inset = bBlocked ? 0.94f : 0.86f;
    return FTransform(
        FRotator::ZeroRotator,
        FVector(
            (static_cast<float>(TileX) - HalfWidth) * WorldUnitsPerTile,
            (static_cast<float>(TileY) - HalfHeight) * WorldUnitsPerTile,
            Height * 0.5f),
        FVector(
            WorldUnitsPerTile * Inset / 100.0f,
            WorldUnitsPerTile * Inset / 100.0f,
            Height / 100.0f));
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
    float TileWorldSize)
{
    if (CubeMesh == nullptr || BasicMaterial == nullptr || TileWorldSize <= 0.0f)
    {
        return false;
    }

    MapWidthTiles = Simulation.Config().mapWidthTiles;
    MapHeightTiles = Simulation.Config().mapHeightTiles;
    WorldUnitsPerTile = TileWorldSize;
    const int32 TileCount = MapWidthTiles * MapHeightTiles;
    CachedTerrain.Init(255, TileCount);

    BlockedTiles->SetStaticMesh(CubeMesh);
    ScarredTiles->SetStaticMesh(CubeMesh);
    BlockedMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
    ScarredMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
    if (BlockedMaterial == nullptr || ScarredMaterial == nullptr)
    {
        return false;
    }
    BlockedMaterial->SetVectorParameterValue(
        ColorParameterName,
        FLinearColor(0.12f, 0.055f, 0.045f));
    ScarredMaterial->SetVectorParameterValue(
        ColorParameterName,
        FLinearColor(0.16f, 0.075f, 0.025f));
    BlockedTiles->SetMaterial(0, BlockedMaterial);
    ScarredTiles->SetMaterial(0, ScarredMaterial);

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
    return SyncTerrain(Simulation);
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
