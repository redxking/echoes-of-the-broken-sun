#include "EchoesFogView.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName ColorParameterName(TEXT("Color"));
}

AEchoesFogView::AEchoesFogView()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    UnexploredTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("UnexploredTiles"));
    UnexploredTiles->SetupAttachment(SceneRoot);
    ExploredTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("ExploredTiles"));
    ExploredTiles->SetupAttachment(SceneRoot);

    for (UInstancedStaticMeshComponent* Layer : {UnexploredTiles, ExploredTiles})
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
    Tags.Add(TEXT("EchoesFogView"));
}

FTransform AEchoesFogView::TileTransform(int32 TileX, int32 TileY) const
{
    const float HalfWidth = static_cast<float>(MapWidthTiles) * 0.5f;
    const float HalfHeight = static_cast<float>(MapHeightTiles) * 0.5f;
    return FTransform(
        FRotator::ZeroRotator,
        FVector(
            (static_cast<float>(TileX) - HalfWidth) * WorldUnitsPerTile,
            (static_cast<float>(TileY) - HalfHeight) * WorldUnitsPerTile,
            14.0f),
        FVector(
            WorldUnitsPerTile / 100.0f,
            WorldUnitsPerTile / 100.0f,
            0.06f));
}

FTransform AEchoesFogView::HiddenTransform()
{
    return FTransform(
        FRotator::ZeroRotator,
        FVector::ZeroVector,
        FVector::ZeroVector);
}

bool AEchoesFogView::InitializeFog(
    const echoes::sim::Simulation& Simulation,
    echoes::sim::PlayerId Player,
    float TileWorldSize)
{
    if (CubeMesh == nullptr || BasicMaterial == nullptr ||
        Simulation.FindPlayer(Player) == nullptr || TileWorldSize <= 0.0f)
    {
        return false;
    }

    PlayerId = Player;
    MapWidthTiles = Simulation.Config().mapWidthTiles;
    MapHeightTiles = Simulation.Config().mapHeightTiles;
    WorldUnitsPerTile = TileWorldSize;
    const int32 TileCount = MapWidthTiles * MapHeightTiles;
    CachedVisibility.Init(255, TileCount);

    UnexploredTiles->SetStaticMesh(CubeMesh);
    ExploredTiles->SetStaticMesh(CubeMesh);
    UnexploredMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
    ExploredMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
    if (UnexploredMaterial == nullptr || ExploredMaterial == nullptr)
    {
        return false;
    }
    UnexploredMaterial->SetVectorParameterValue(
        ColorParameterName,
        FLinearColor(0.001f, 0.003f, 0.008f));
    ExploredMaterial->SetVectorParameterValue(
        ColorParameterName,
        FLinearColor(0.018f, 0.032f, 0.052f));
    UnexploredTiles->SetMaterial(0, UnexploredMaterial);
    ExploredTiles->SetMaterial(0, ExploredMaterial);

    UnexploredTiles->ClearInstances();
    ExploredTiles->ClearInstances();
    UnexploredTiles->PreAllocateInstancesMemory(TileCount);
    ExploredTiles->PreAllocateInstancesMemory(TileCount);
    const FTransform Hidden = HiddenTransform();
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            UnexploredTiles->AddInstance(Hidden, false);
            ExploredTiles->AddInstance(Hidden, false);
        }
    }
    return SyncVisibility(Simulation);
}

bool AEchoesFogView::SyncVisibility(
    const echoes::sim::Simulation& Simulation)
{
    if (Simulation.Config().mapWidthTiles != MapWidthTiles ||
        Simulation.Config().mapHeightTiles != MapHeightTiles ||
        CachedVisibility.Num() != MapWidthTiles * MapHeightTiles ||
        UnexploredTiles->GetInstanceCount() != CachedVisibility.Num() ||
        ExploredTiles->GetInstanceCount() != CachedVisibility.Num())
    {
        return false;
    }

    UnexploredTileCount = 0;
    ExploredTileCount = 0;
    VisibleTileCount = 0;
    bool bChanged = false;
    const FTransform Hidden = HiddenTransform();
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            const int32 TileIndex = TileY * MapWidthTiles + TileX;
            const echoes::sim::Visibility Visibility = Simulation.VisibilityAt(
                PlayerId,
                echoes::sim::Vec2::FromTiles(TileX, TileY));
            const uint8 Encoded = static_cast<uint8>(Visibility);
            UnexploredTileCount +=
                Visibility == echoes::sim::Visibility::Unexplored ? 1 : 0;
            ExploredTileCount +=
                Visibility == echoes::sim::Visibility::Explored ? 1 : 0;
            VisibleTileCount +=
                Visibility == echoes::sim::Visibility::Visible ? 1 : 0;
            if (CachedVisibility[TileIndex] == Encoded)
            {
                continue;
            }
            CachedVisibility[TileIndex] = Encoded;
            const FTransform VisibleTransform = TileTransform(TileX, TileY);
            UnexploredTiles->UpdateInstanceTransform(
                TileIndex,
                Visibility == echoes::sim::Visibility::Unexplored
                    ? VisibleTransform
                    : Hidden,
                false,
                false,
                true);
            ExploredTiles->UpdateInstanceTransform(
                TileIndex,
                Visibility == echoes::sim::Visibility::Explored
                    ? VisibleTransform
                    : Hidden,
                false,
                false,
                true);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        UnexploredTiles->MarkRenderStateDirty();
        ExploredTiles->MarkRenderStateDirty();
    }
    return true;
}
